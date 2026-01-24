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

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{
	struct AddrInfoGuard
	{
		addrinfo *res;
		AddrInfoGuard() : res(0) {}
		~AddrInfoGuard() { if (res) freeaddrinfo(res); }

	private:
		AddrInfoGuard(const AddrInfoGuard &);
		AddrInfoGuard &operator=(const AddrInfoGuard &);
	};

	void respondError(Client &conn, int status, std::string message)
	{
		conn.out_buf += buildErrorResponse(status, conn.keep_alive, message);
		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	void respondText(Client &conn, int status, const char *body)
	{
		conn.out_buf += buildResponse(status, body, conn.keep_alive, "text/plain");
		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	bool handleUpload(Client &conn, const Location *loc, const std::string &uri)
	{
		if (conn.request.method_enum != METHOD_POST && conn.request.method_enum != METHOD_PUT)
			return false;

		std::string upload_root;
		if (loc)
			upload_root = loc->getUploadPath();
		if (upload_root.empty())
		{
			respondError(conn, 403, "upload_root.empty()");
			return true;
		}
		if (!isDirectory(upload_root))
		{
			respondError(conn, 500, "!isDirectory(upload_root)");
			return true;
		}
		std::string name = serverutil::lastPathSegment(uri);
		if (name.empty())
			name = serverutil::buildUploadName();
		std::string out_path = joinPath(upload_root, name);
		if (!serverutil::isPathWithinRoot(upload_root, out_path))
		{
			respondError(conn, 403, "!serverutil::isPathWithinRoot(upload_root, out_path)");
			return true;
		}
		if (!writeFile(out_path, conn.request.body))
		{
			respondError(conn, 500, "!writeFile(out_path, conn.request.body)");
			return true;
		}
		respondText(conn, 201, "Created\n");
		return true;
	}

	bool handleDelete(Client &conn, const Location *loc, const std::string &root,
					  const std::string &path, const std::string &uri)
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
			respondError(conn, 403, "delete_root.empty()");
			return true;
		}
		if (use_upload_root && !isDirectory(delete_root))
		{
			respondError(conn, 500, "use_upload_root && !isDirectory(delete_root)");
			return true;
		}

		std::string delete_path;
		if (use_upload_root)
		{
			std::string name = serverutil::lastPathSegment(uri);
			if (name.empty())
			{
				respondError(conn, 403, "name.empty()");
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
			respondError(conn, 403, "!serverutil::isPathWithinRoot(delete_root, delete_path)");
			return true;
		}

		if (isDirectory(delete_path))
		{
			respondError(conn, 403, "isDirectory(delete_path)");
			return true;
		}
		if (!isFile(delete_path))
		{
			respondError(conn, 404, "!isFile(delete_path)");
			return true;
		}
		if (!deleteFile(delete_path))
		{
			respondError(conn, 500, "!deleteFile(delete_path)");
			return true;
		}
		respondText(conn, 200, "OK\n");
		return true;
	}

	void serveStatic(Client &conn, const std::string &path, const std::string &uri,
					 const std::string &index, bool autoindex)
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
				resp_status = 403;
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
		}

		if (resp_status != 200)
			respondError(conn, resp_status, "resp_status != 200");
		else
			conn.out_buf += buildResponse(200, body, conn.keep_alive, content_type);

		conn.state = Client::WRITING;
		serverutil::resetRequest(conn);
	}

	void serveCgi(Client &connection, const Config &cfg)
	{
		CgiHandler handler;

		// Determine interpreter based on file extension
		LOG_DBG << "HERE";
		std::string ext = getFileExtension(connection.request.file_name);
		LOG_DBG << "HERE";
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

		// Execute CGI script
		std::string script_name = handler.getScriptName(connection.request.file_name);
		std::string cgi_output = handler.handleRequest(connection, connection.cgi_script_path + "/" + script_name);

		if (handler.hasError())
		{
			respondError(connection, 500, "handler.hasError()");
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
		connection.out_buf += buildCgiResponse(headers, body, connection.keep_alive);
		connection.state = Client::WRITING;
		serverutil::resetRequest(connection);
		connection.resetCgiInfo();
	}
}

// Constructor
Worker::Worker(const Config &config) : _listening_fd(-1), _config(config)
{
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
	port << _config.getPort();
	if (getaddrinfo(_config.getHost().c_str(), port.str().c_str(), &hints, &info.res) != 0)
		throw std::runtime_error("getaddrinfo failed");

	for (struct addrinfo *p = info.res; p != 0; p = p->ai_next)
	{
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

		_listening_fd = sock.release();
		return; // Successfully created listening socket
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
				conn.out_buf += buildErrorResponse(status, false, "Parser::PARSE_ERROR");
				conn.state = Client::WRITING;
				break;
			}

			if (conn.request.has_body
				&& _config.getClientMaxBodySize() > 0
				&& conn.request.content_length > static_cast<size_t>(_config.getClientMaxBodySize()))
			{
				conn.keep_alive = false;
				conn.out_buf += buildErrorResponse(413, false, "MAX BODY SIZE exceeded");
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
	std::string uri = stripQuery(connection.request.target).first;
	std::string request = stripQuery(connection.request.target).second;
	connection.request.file_name = stripFilename(request).first;
	connection.request.query = stripFilename(connection.request.target).second;

	LOG_DBG << "filename: " << connection.request.file_name;

//	connection.cgi_script_path = connection.request.file_name;
	LOG_DBG << "conn.request.query: " << connection.request.query;
	if (uri.empty())
		uri = "/";

	LOG_DBG << "handleReadyRequest uri: " << uri;
	Location loc = matchLocation(_config, uri);
	LOG_DBG << "handleReadyRequest matchLocation result: " << loc.getCgiBinPath();
	_config.logDebug();

	if (!serverutil::isMethodAllowed(loc.getAllowedMethods(), connection.request.method))
	{
		respondError(connection, 405, "loc && !serverutil::isMethodAllowed(loc->getAllowedMethods(), conn.request.method)");
		return;
	}

	if (hasTraversal(uri))
	{
		respondError(connection, 403, "hasTraversal(uri)");
		return;
	}

	std::string root = _config.getRoot();
	std::string index = _config.getIndex();
	bool autoindex = false;
	std::string alias;

	if (!loc.getRoot().empty())
		root = loc.getRoot();
	if (!loc.getIndex().empty())
		index = loc.getIndex();
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

	bool upload = false;
	if (upload)
	{
		handleUpload(connection, &loc, uri);
		return;
	}
	if (handleDelete(connection, &loc, root, path, uri))
	{
		return;
	}

	std::vector<std::string> llc = connection.location->getCgiPath();
	for (size_t i = 0; i < llc.size(); i++) {
		LOG_DBG << "llc" << llc[i];
	}

	LOG_DBG << "request: " << connection.request.target << " " << connection.request.file_name << " " << connection.request.query << " " << index << " " << autoindex;

	if (!connection.cgi_request)
		serveStatic(connection, path, uri, index, autoindex);
	else
		serveCgi(connection, _config);
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
