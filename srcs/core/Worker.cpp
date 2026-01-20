#include "core/Worker.hpp"
#include "core/ServerInternal.hpp"
#include "core/Fd.hpp"
#include "http/Parser.hpp"
#include "http/ResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include "utils/Path.hpp"
#include "utils/Time.hpp"
#include "io/FileSystem.hpp"
#include "io/NonBlocking.hpp"
#include "config/Route.hpp"
#include "cgi/CgiHandler.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{
	std::string dirName(const std::string &path)
	{
		if (path.empty())
			return "";
		std::string::size_type end = path.size();
		while (end > 1 && path[end - 1] == '/')
			--end;
		std::string::size_type slash = path.find_last_of('/', end - 1);
		if (slash == std::string::npos)
			return "";
		if (slash == 0)
			return "/";
		return path.substr(0, slash);
	}

	std::string pickErrorBaseDir(const Config &cfg)
	{
		std::string root = cfg.getRoot();
		ErrorPage ep = cfg.getErrorPage();
		std::string desired_dir = ep.path.empty() ? "errors" : dirName(ep.path);

		if (!ep.path.empty())
		{
			if (!ep.path.empty() && ep.path[0] == '/')
				return desired_dir;
			std::string root_trim = root;
			if (!root_trim.empty() && root_trim[root_trim.size() - 1] == '/')
				root_trim = root_trim.substr(0, root_trim.size() - 1);
			if (!root_trim.empty()
				&& ep.path.size() >= root_trim.size()
				&& ep.path.compare(0, root_trim.size(), root_trim) == 0)
				return desired_dir;
		}

		std::string candidate = joinPath(root, desired_dir);
		if (!candidate.empty() && isDirectory(candidate))
			return candidate;

		std::string parent = dirName(root);
		if (!parent.empty())
		{
			std::string parent_candidate = joinPath(parent, desired_dir);
			if (!parent_candidate.empty() && isDirectory(parent_candidate))
				return parent_candidate;
		}

		return candidate;
	}

	bool ensureDirExists(const std::string &path)
	{
		if (path.empty())
			return false;
		if (isDirectory(path))
			return true;
		if (isFile(path))
			return false;
		return mkdir(path.c_str(), 0755) == 0;
	}

	std::string resolveErrorPagePath(const Config &cfg)
	{
		ErrorPage ep = cfg.getErrorPage();
		if (ep.path.empty())
			return "";
		if (ep.path[0] == '/')
			return ep.path;
		std::string root = cfg.getRoot();
		std::string root_trim = root;
		if (!root_trim.empty() && root_trim[root_trim.size() - 1] == '/')
			root_trim = root_trim.substr(0, root_trim.size() - 1);
		if (!root_trim.empty()
			&& ep.path.size() >= root_trim.size()
			&& ep.path.compare(0, root_trim.size(), root_trim) == 0)
			return ep.path;
		return joinPath(root, ep.path);
	}

	std::string normalizeHostHeader(const std::string &host_header)
	{
		std::string host = serverutil::trim(host_header);
		std::string::size_type colon = host.find(':');
		if (colon != std::string::npos)
			host = host.substr(0, colon);
		return serverutil::toLower(host);
	}

	size_t selectConfigIndex(const std::vector<Config> &configs, const Request &req)
	{
		if (configs.empty())
			return 0;
		std::map<std::string, std::string>::const_iterator it = req.headers.find("host");
		if (it == req.headers.end())
			return 0;
		std::string want = normalizeHostHeader(it->second);
		if (want.empty())
			return 0;

		for (size_t i = 0; i < configs.size(); ++i)
		{
			std::vector<std::string> names = configs[i].getServerNames();
			if (names.empty() && !configs[i].getServerName().empty())
				names.push_back(configs[i].getServerName());
			for (size_t n = 0; n < names.size(); ++n)
			{
				if (serverutil::toLower(names[n]) == want)
					return i;
			}
		}
		return 0;
	}

	struct AddrInfoGuard
	{
		addrinfo *res;
		AddrInfoGuard() : res(0) {}
		~AddrInfoGuard() { if (res) freeaddrinfo(res); }

	private:
		AddrInfoGuard(const AddrInfoGuard &);
		AddrInfoGuard &operator=(const AddrInfoGuard &);
	};

	void respondError(Client &conn, int status, std::string message, ErrorPages *pages, bool head_only)
	{
		conn.out_buf += buildErrorResponse(status, conn.keep_alive, message, pages, !head_only);
		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	std::string formatParseError(const std::string &err)
	{
		if (err.empty())
			return "Parser error";
		return "Parser error: " + err;
	}

	void respondText(Client &conn, int status, const char *body, bool head_only)
	{
		conn.out_buf += buildResponse(status, body, conn.keep_alive, "text/plain", !head_only);
		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	bool handleUpload(Client &conn, const Location *loc, const std::string &uri, ErrorPages *pages, bool head_only)
	{
		if (conn.request.method_enum != METHOD_POST && conn.request.method_enum != METHOD_PUT)
			return false;

		std::string upload_root;
		if (loc)
			upload_root = loc->getUploadPath();
		if (upload_root.empty())
			return false;
		if (!isDirectory(upload_root))
		{
			respondError(conn, 500, "!isDirectory(upload_root)", pages, head_only);
			return true;
		}
		std::string name = serverutil::lastPathSegment(uri);
		if (name.empty())
			name = serverutil::buildUploadName();
		std::string out_path = joinPath(upload_root, name);
		if (!serverutil::isPathWithinRoot(upload_root, out_path))
		{
			respondError(conn, 403, "!serverutil::isPathWithinRoot(upload_root, out_path)", pages, head_only);
			return true;
		}
		if (!writeFile(out_path, conn.request.body))
		{
			respondError(conn, 500, "!writeFile(out_path, conn.request.body)", pages, head_only);
			return true;
		}
		respondText(conn, 201, "Created\n", head_only);
		return true;
	}

	bool handleDelete(Client &conn, const Location *loc, const std::string &root,
					  const std::string &path, const std::string &uri, ErrorPages *pages, bool head_only)
	{
		if (conn.request.method_enum != METHOD_DELETE)
			return false;

		std::string delete_root = root;
		bool use_upload_root = false;
		if (loc && !loc->getUploadPath().empty())
		{
			delete_root = loc->getUploadPath();
			use_upload_root = true;
		}
		if (delete_root.empty())
		{
			respondError(conn, 403, "delete_root.empty()", pages, head_only);
			return true;
		}
		if (use_upload_root && !isDirectory(delete_root))
		{
			respondError(conn, 500, "use_upload_root && !isDirectory(delete_root)", pages, head_only);
			return true;
		}

		std::string delete_path;
		if (use_upload_root)
		{
			std::string name = serverutil::lastPathSegment(uri);
			if (name.empty())
			{
				respondError(conn, 403, "name.empty()", pages, head_only);
				return true;
			}
			delete_path = joinPath(delete_root, name);
		}
		else
		{
			delete_path = path;
		}
		if (!serverutil::isPathWithinRoot(delete_root, delete_path))
		{
			respondError(conn, 403, "!serverutil::isPathWithinRoot(delete_root, delete_path)", pages, head_only);
			return true;
		}

		if (isDirectory(delete_path))
		{
			respondError(conn, 403, "isDirectory(delete_path)", pages, head_only);
			return true;
		}
		if (!isFile(delete_path))
		{
			respondError(conn, 404, "!isFile(delete_path)", pages, head_only);
			return true;
		}
		if (!deleteFile(delete_path))
		{
			respondError(conn, 500, "!deleteFile(delete_path)", pages, head_only);
			return true;
		}
		respondText(conn, 200, "OK\n", head_only);
		return true;
	}

	void serveStatic(Client &conn, const Request &req, const std::string &path, const std::string &uri,
					 const std::string &index, bool autoindex, ErrorPages *pages, bool head_only)
	{
		std::string body;
		std::string content_type = "text/plain";
		int resp_status = 200;
		bool ok = false;

		if (isDirectory(path))
		{
			if (!index.empty())
			{
				std::string idx_path = joinPath(path, index);
				if (isFile(idx_path))
				{
					body = readFile(idx_path, ok);
					content_type = contentTypeForPath(idx_path);
				}
			}

			if (body.empty() && autoindex)
			{
				body = buildAutoindex(path, uri);
				if (body.empty())
					resp_status = 500;
				content_type = "text/html";
				ok = !body.empty();
			}

			if (body.empty() && !autoindex)
				resp_status = 404;
		}
		else if (isFile(path))
		{
			body = readFile(path, ok);
			content_type = contentTypeForPath(path);
			if (!ok)
				resp_status = 500;
		}
		else
		{
			resp_status = 404;

			std::string::size_type dot = path.find_last_of('.');
			if (dot == std::string::npos)
			{
				std::string base_html = path + ".html";
				std::string lang;
				std::string cand;
				std::map<std::string, std::string>::const_iterator it_lang = req.headers.find("accept-language");
				if (it_lang != req.headers.end())
				{
					std::string v = serverutil::toLower(it_lang->second);
					if (v.find("fr") != std::string::npos)
					{
						lang = "fr";
						cand = base_html + ".fr";
					}
					else if (v.find("en") != std::string::npos)
					{
						lang = "en";
						cand = path + ".en.html";
					}
				}

				std::map<std::string, std::string>::const_iterator it_cs = req.headers.find("accept-charset");
				bool wants_utf8 = false;
				if (it_cs != req.headers.end())
				{
					std::string v = serverutil::toLower(it_cs->second);
					if (v.find("utf-8") != std::string::npos)
						wants_utf8 = true;
				}

				std::string chosen = cand;
				std::string lang_header = lang;
				std::string chosen_type = "text/html";
				if (chosen.empty() && wants_utf8)
				{
					std::string utf8_cand = path + ".en.html.utf-8";
					if (isFile(utf8_cand))
					{
						chosen = utf8_cand;
						chosen_type = "text/html; charset=utf-8";
					}
				}
				if (chosen.empty() && isFile(base_html))
				{
					chosen = base_html;
					chosen_type = "text/html";
				}
				if (!chosen.empty() && isFile(chosen))
				{
					body = readFile(chosen, ok);
					if (ok)
					{
						content_type = chosen_type;
						resp_status = 200;
						std::map<std::string, std::string> extra;
						if (!lang_header.empty())
							extra["Content-Language"] = lang_header;
						if (resp_status != 200)
							respondError(conn, resp_status, "resp_status != 200", pages, head_only);
						else
							conn.out_buf += buildResponse(200, body, conn.keep_alive, content_type, !head_only, extra);
						conn.state = Client::WRITING;
						serverutil::resetRequest(conn);
						return;
					}
				}
			}
		}

		if (resp_status != 200)
			respondError(conn, resp_status, "resp_status != 200", pages, head_only);
		else
			conn.out_buf += buildResponse(200, body, conn.keep_alive, content_type, !head_only);

		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	void serveCgi(Client &connection, const Config &cfg, ErrorPages *pages, bool head_only)
	{
		CgiHandler handler;

		// Determine interpreter based on file extension
		std::string ext = getFileExtension(connection.request.file_name);
		LOG_DBG << "cgi: ext=" << ext;
		if (ext == ".py")
		{
			handler.setPythonInterpreter("/usr/bin/python3");
			connection.request.cgi = Python;
		}
		else if (ext == ".php")
		{
			handler.setPhpInterpreter("/usr/bin/php");
			connection.request.cgi = PHP;
		}
		else if (ext == ".sh")
		{
			handler.setPythonInterpreter("/bin/sh");
			connection.request.cgi = Shell;
		}
		else
		{
			// Default to Python or make it executable directly
			handler.setPythonInterpreter("/usr/bin/python3");
			connection.request.cgi = Python;
		}

		// Set CGI environment from configuration
		handler.setDocumentRoot(cfg.getRoot());
		handler.setServerName(cfg.getServerName());
		handler.setServerPort(cfg.getPort());

		// Execute CGI script (cgi_script_path already points to the script file)
		std::string cgi_output = handler.handleRequest(connection, connection.cgi_script_path);

		if (handler.hasError())
		{
			respondError(connection, 500, "handler.hasError()", pages, head_only);
			return;
		}

		// Parse CGI output to separate headers and body
		std::string headers;
		std::string body;
		size_t header_end = cgi_output.find("\r\n\r\n");
		if (header_end == std::string::npos)
		{
			header_end = cgi_output.find("\n\n");
			if (header_end != std::string::npos)
			{
				headers = cgi_output.substr(0, header_end);
				body = cgi_output.substr(header_end + 2);
			}
			else
			{
				// No headers found, treat all as body
				body = cgi_output;
			}
		}
		else
		{
			headers = cgi_output.substr(0, header_end);
			body = cgi_output.substr(header_end + 4);
		}

		// Build response
		connection.out_buf += buildCgiResponse(headers, body, connection.keep_alive, !head_only);
		connection.state = Client::WRITING;
		serverutil::resetRequest(connection);
		connection.resetCgiInfo();
	}

	bool handleWriteToRoot(Client &conn, const std::string &root, const std::string &uri,
						   ErrorPages *pages, bool head_only)
	{
		if (conn.request.method_enum != METHOD_PUT && conn.request.method_enum != METHOD_POST)
			return false;

		if (root.empty())
		{
			respondError(conn, 403, "root.empty()", pages, head_only);
			return true;
		}

		std::string out_path = joinPath(root, uri);
		std::string::size_type slash = out_path.find_last_of('/');
		if (slash != std::string::npos)
		{
			std::string parent = out_path.substr(0, slash);
			if (!ensureDirExists(parent))
			{
				respondError(conn, 403, "cannot create parent directory", pages, head_only);
				return true;
			}
		}
		if (!serverutil::isPathWithinRoot(root, out_path))
		{
			respondError(conn, 403, "!serverutil::isPathWithinRoot(root, out_path)", pages, head_only);
			return true;
		}
		if (isDirectory(out_path))
		{
			respondError(conn, 403, "isDirectory(out_path)", pages, head_only);
			return true;
		}

		bool existed = isFile(out_path);
		if (!writeFile(out_path, conn.request.body))
		{
			respondError(conn, 500, "!writeFile(out_path, conn.request.body)", pages, head_only);
			return true;
		}

		int status = existed ? 204 : 201;
		respondText(conn, status, existed ? "No Content\n" : "Created\n", head_only);
		return true;
	}
}

// Constructor
Worker::Worker(const std::vector<Config> &configs)
	: _listening_fd(-1),
	  _configs(configs),
	  _error_pages()
{
	if (_configs.empty())
		throw std::runtime_error("Worker created with no configs");
	_error_pages.resize(_configs.size());
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		_error_pages[i].setBaseDir(pickErrorBaseDir(_configs[i]));
		ErrorPage ep = _configs[i].getErrorPage();
		if (ep.code > 0)
		{
			std::string resolved = resolveErrorPagePath(_configs[i]);
			if (!resolved.empty())
				_error_pages[i].setOverride(ep.code, resolved);
		}
	}
	initListeningSocket();
}

// Destructor
Worker::~Worker()
{
	for (std::map<int, Client>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		close(it->first);
	}
	if (_listening_fd >= 0)
		close(_listening_fd);
}

// Initialize listening socket from config
void Worker::initListeningSocket()
{
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	AddrInfoGuard info;
	std::stringstream port;
	const Config &cfg = _configs[0];
	port << cfg.getPort();
	std::string host_cfg = cfg.getHost();
	const char *host = host_cfg.empty() ? 0 : host_cfg.c_str();
	if (getaddrinfo(host, port.str().c_str(), &hints, &info.res) != 0)
		throw std::runtime_error("getaddrinfo failed");

	bool tried_any = false;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (struct addrinfo *p = info.res; p != 0; p = p->ai_next)
		{
			if (pass == 0 && p->ai_family != AF_INET)
				continue;
			if (pass == 1 && p->ai_family == AF_INET)
				continue;

			Fd sock(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
			if (sock.get() < 0)
			{
				LOG_ERR << "socket() failed: " << std::strerror(errno);
				continue;
			}

			int opt = 1;
			if (setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			{
				LOG_ERR << "setsockopt(SO_REUSEADDR) failed: " << std::strerror(errno);
				continue;
			}

			int client_fd = sock.get();
			if (makeNonBlocking(client_fd) == -1)
			{
				LOG_ERR << "makeNonBlocking failed: " << std::strerror(errno);
				close(client_fd);
				continue;
			}

			if (bind(sock.get(), p->ai_addr, p->ai_addrlen) < 0)
			{
				LOG_ERR << "bind() failed on port " << cfg.getPort() << ": " << std::strerror(errno);
				continue;
			}

			if (listen(sock.get(), 128) < 0)
			{
				LOG_ERR << "listen() failed: " << std::strerror(errno);
				continue;
			}

			_listening_fd = sock.release();
			return; // Successfully created listening socket
		}
	}

	if (!host_cfg.empty() && !tried_any)
	{
		tried_any = true;
		freeaddrinfo(info.res);
		info.res = 0;
		if (getaddrinfo(0, port.str().c_str(), &hints, &info.res) == 0)
		{
			for (int pass = 0; pass < 2; ++pass)
			{
				for (struct addrinfo *p = info.res; p != 0; p = p->ai_next)
				{
					if (pass == 0 && p->ai_family != AF_INET)
						continue;
					if (pass == 1 && p->ai_family == AF_INET)
						continue;

					Fd sock(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
					if (sock.get() < 0)
						continue;

					int opt = 1;
					if (setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
						continue;

					int client_fd = sock.get();
					if (makeNonBlocking(client_fd) == -1)
					{
						close(client_fd);
						continue;
					}

					if (bind(sock.get(), p->ai_addr, p->ai_addrlen) < 0)
						continue;

					if (listen(sock.get(), 128) < 0)
						continue;

					LOG_WRN << "bind() failed for host " << host_cfg << "; using 0.0.0.0 instead";
					_listening_fd = sock.release();
					return;
				}
			}
		}
	}

	throw std::runtime_error("No listening sockets available");
}

// Accept new client connection
void Worker::acceptNewClient()
{
	while (true)
	{
		int client_fd = accept(_listening_fd, 0, 0);
		if (client_fd < 0)
			break;

		if (makeNonBlocking(client_fd) == -1)
		{
			close(client_fd);
			continue;
		}

		Client conn(client_fd);
		conn.last_activity_ms = now_ms();
		conn.header_start_ms = conn.last_activity_ms;
		conn.config_index = 0; // Worker only has one config
		_clients.insert(std::make_pair(client_fd, conn));
	}
}

// Close client connection
void Worker::closeClient(int fd)
{
	close(fd);
	_clients.erase(fd);
}

// Handle reading from client
void Worker::handleClientRead(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client &conn = it->second;
	char buffer[4096];

	for (;;)
	{
		ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

		if (n > 0)
		{
			conn.in_buf.append(buffer, static_cast<size_t>(n));
			conn.last_activity_ms = now_ms();
			continue;
		}

		if (n == 0)
		{
			closeClient(fd);
			return;
		}

		if (errno == EINTR)
			continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		closeClient(fd);
		return;
	}

	if (!conn.in_buf.empty())
	{
		Parser parser;
		for (;;)
		{
			if (conn.state == Client::READING_BODY)
			{
				size_t remaining = conn.body_bytes_expected - conn.body_bytes_read;
				if (remaining == 0)
				{
					conn.state = Client::READING_HEADERS;
				}
				else
				{
					size_t take = remaining;
					if (take > conn.in_buf.size())
						take = conn.in_buf.size();
					conn.request.body.append(conn.in_buf, 0, take);
					conn.in_buf.erase(0, take);
					conn.body_bytes_read += take;
					if (conn.body_bytes_read < conn.body_bytes_expected)
						break;
					conn.state = Client::READING_HEADERS;
				}
				handleReadyRequest(conn);
				if (conn.state == Client::WRITING)
					break;
				continue;
			}

			int status = 200;
			std::string err;
			Parser::Result result = parser.parseOne(conn.in_buf, conn.request, status, err);
			if (result == Parser::NEED_MORE)
				break;

			if (conn.request.version == "HTTP/1.1")
				conn.keep_alive = true;
			else
				conn.keep_alive = false;

			std::map<std::string, std::string>::iterator it =
					conn.request.headers.find("connection");
			if (it != conn.request.headers.end())
			{
				std::string conn_val = serverutil::toLower(it->second);
				if (conn_val == "close")
					conn.keep_alive = false;
				else if (conn_val == "keep-alive")
					conn.keep_alive = true;
			}

			if (result == Parser::PARSE_ERROR || status != 200)
			{
				conn.keep_alive = false;
				size_t idx = conn.config_index;
				if (idx >= _error_pages.size())
					idx = 0;
				conn.out_buf += buildErrorResponse(status, false, formatParseError(err), &_error_pages[idx]);
				conn.state = Client::WRITING;
				break;
			}

			conn.config_index = selectConfigIndex(_configs, conn.request);
			const Config &cfg = _configs[conn.config_index];

			size_t max_body = 0;
			if (cfg.getClientMaxBodySize() > 0)
				max_body = static_cast<size_t>(cfg.getClientMaxBodySize());
			std::pair<std::string, std::string> uri_query = stripQuery(conn.request.target);
			Location loc = matchLocation(cfg, uri_query.first);
			if (loc.getClientMaxBodySize() > 0)
				max_body = static_cast<size_t>(loc.getClientMaxBodySize());

			if (conn.request.has_body
				&& max_body > 0
				&& conn.request.content_length > max_body)
			{
				conn.keep_alive = false;
				conn.out_buf += buildErrorResponse(413, false, "MAX BODY SIZE exceeded", &_error_pages[conn.config_index]);
				conn.state = Client::WRITING;
				serverutil::resetRequest(conn);
				break;
			}

			if (conn.request.has_body)
			{
				conn.body_bytes_expected = conn.request.content_length;
				conn.body_bytes_read = 0;
				conn.state = Client::READING_BODY;
				continue;
			}

			handleReadyRequest(conn);
			if (conn.state == Client::WRITING)
				break;
		}
	}
}

// Handle writing to client
void Worker::handleClientWrite(int fd)
{
	std::map<int, Client>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;

	Client &conn = it->second;
	ssize_t n = send(fd, conn.out_buf.c_str(), conn.out_buf.size(), 0);
	if (n < 0)
	{
		if (errno == EINTR)
			return;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		closeClient(fd);
		return;
	}
	if (n == 0)
	{
		closeClient(fd);
		return;
	}

	conn.out_buf.erase(0, static_cast<size_t>(n));
	conn.last_activity_ms = now_ms();
	if (conn.out_buf.empty())
	{
		if (conn.keep_alive)
		{
			conn.header_start_ms = now_ms();
			conn.state = Client::READING_HEADERS;
		}
		else
			closeClient(fd);
	}
}

// Handle a ready HTTP request
void Worker::handleReadyRequest(Client &connection)
{
	std::pair<std::string, std::string> uri_query = stripQuery(connection.request.target);
	std::string uri = uri_query.first;
	connection.request.query = uri_query.second;
	connection.request.file_name = stripFilename(uri).first;

	LOG_DBG << "request: target=" << connection.request.target
			<< " uri=" << uri
			<< " file=" << connection.request.file_name
			<< " query=" << connection.request.query;
	if (uri.empty())
		uri = "/";

	const Config &cfg = _configs[connection.config_index];
	LOG_DBG << "handleReadyRequest uri: " << uri;
	Location loc = matchLocation(cfg, uri);
	LOG_DBG << "handleReadyRequest matchLocation result: " << loc.getCgiBinPath();
	cfg.logDebug();

	if (!serverutil::isMethodAllowed(loc.getAllowedMethods(), connection.request.method))
	{
		respondError(connection, 405, "loc && !serverutil::isMethodAllowed(loc->getAllowedMethods(), conn.request.method)",
					&_error_pages[connection.config_index], connection.request.method_enum == METHOD_HEAD);
		return;
	}

	if (hasTraversal(uri))
	{
		respondError(connection, 403, "hasTraversal(uri)", &_error_pages[connection.config_index],
					connection.request.method_enum == METHOD_HEAD);
		return;
	}

	std::string root = cfg.getRoot();
	std::string index = cfg.getIndex();
	bool autoindex = false;
	std::string alias;

	if (!loc.getRoot().empty())
		root = loc.getRoot();
	if (!loc.getIndex().empty())
		index = loc.getIndex();
	else if (!loc.getRoot().empty() && loc.getPath() != "/")
		index.clear();
	if (!loc.getAlias().empty())
		alias = loc.getAlias();
	autoindex = loc.getAutoindex();

	std::string path;
	if (!alias.empty())
	{
		// Alias replaces the location path prefix
		std::string remainder = uri.substr(loc.getPath().size());
		path = joinPath(alias, remainder);
		LOG_DBG << "!alias.empty() " << path;
	}
	else if (!loc.getRoot().empty() && loc.getPath() != "/")
	{
		std::string remainder = uri.substr(loc.getPath().size());
		path = joinPath(root, remainder);
	}
	else
	{
		path = joinPath(root, uri);
	}

	connection.cgi_request = false;  // Reset first
	connection.location = &loc;       // Store location pointer

	if (loc.isCgiEnabled())
	{
		// Check if request targets a CGI script
		std::string ext = getFileExtension(connection.request.file_name);
		LOG_DBG << "ext " << ext;
		std::vector<std::string> cgi_exts = loc.getCgiExt();
		for (size_t i = 0; i < cgi_exts.size(); ++i) {
			LOG_DBG << "getCgiExt " << cgi_exts[i];
		}

		// If no extensions configured, allow common CGI extensions
		if (cgi_exts.empty())
		{
			if (ext == ".py" || ext == ".php" || ext == ".sh" || ext == ".cgi")
			{
				connection.cgi_request = true;
			}
			if (ext == ".py")
				connection.request.cgi = Python;
		}
		else
		{
			// Check against configured extensions
			for (size_t i = 0; i < cgi_exts.size(); ++i)
			{
				if (ext == cgi_exts[i])
				{
					connection.cgi_request = true;
					break;
				}
			}
		}

		if (connection.cgi_request)
		{
			connection.cgi_script_path = path;
			LOG_DBG << "LOG_DBG cgi_script_path " << path;
			connection.cgi_bin_path = !alias.empty() ? alias : loc.getCgiBinPath();
			// Extract PATH_INFO if there's additional path after script
			connection.cgi_path_info = ""; // Can be enhanced later
		}
	}

	if (handleUpload(connection, &loc, uri, &_error_pages[connection.config_index], connection.request.method_enum == METHOD_HEAD))
	{
		return;
	}
	if (handleWriteToRoot(connection, root, uri, &_error_pages[connection.config_index],
						  connection.request.method_enum == METHOD_HEAD))
	{
		return;
	}
	if (handleDelete(connection, &loc, root, path, uri, &_error_pages[connection.config_index], connection.request.method_enum == METHOD_HEAD))
	{
		return;
	}

	std::vector<std::string> llc = connection.location->getCgiPath();
	for (size_t i = 0; i < llc.size(); i++) {
		LOG_DBG << "llc" << llc[i];
	}

	LOG_DBG << "request: " << connection.request.target << " " << connection.request.file_name << " " << connection.request.query << " " << index << " " << autoindex;

	if (!connection.cgi_request)
		serveStatic(connection, connection.request, path, uri, index, autoindex, &_error_pages[connection.config_index],
					connection.request.method_enum == METHOD_HEAD);
	else
		serveCgi(connection, cfg, &_error_pages[connection.config_index], connection.request.method_enum == METHOD_HEAD);
}

// Add this worker's fds to poll array
void Worker::addToPollFds(std::vector<struct pollfd> &pfds) const
{
	// Add listening socket
	struct pollfd pfd;
	pfd.fd = _listening_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	pfds.push_back(pfd);

	// Add all client sockets
	for (std::map<int, Client>::const_iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		struct pollfd pfd;
		pfd.fd = it->first;
		short ev = POLLIN;
		if (!it->second.out_buf.empty())
			ev |= POLLOUT;
		pfd.events = ev;
		pfd.revents = 0;
		pfds.push_back(pfd);
	}
}

// Handle a poll event for one of this worker's fds
void Worker::handlePollEvent(const struct pollfd &pfd)
{
	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		if (_clients.find(pfd.fd) != _clients.end())
			closeClient(pfd.fd);
		return;
	}

	if (isListeningFd(pfd.fd))
	{
		if (pfd.revents & POLLIN)
			acceptNewClient();
		return;
	}

	if (pfd.revents & POLLIN)
		handleClientRead(pfd.fd);

	if (_clients.find(pfd.fd) != _clients.end() && (pfd.revents & POLLOUT))
		handleClientWrite(pfd.fd);
}

// Check and handle client timeouts
void Worker::checkTimeouts()
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); )
	{
		Client &conn = it->second;
		if (conn.state == Client::READING_HEADERS
			&& elapsed_ms(conn.header_start_ms) > serverutil::kHeaderTimeoutMs)
		{
			int fd = it->first;
			++it;
			closeClient(fd);
			continue;
		}
		++it;
	}

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); )
	{
		if (elapsed_ms(it->second.last_activity_ms) > serverutil::kIdleTimeoutMs)
		{
			int fd = it->first;
			++it;
			closeClient(fd);
		}
		else
		{
			++it;
		}
	}
}
