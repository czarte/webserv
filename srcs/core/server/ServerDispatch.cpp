#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "http/ResponseBuilder.hpp"
#include "utils/Logger.hpp"
#include "utils/Path.hpp"
#include "io/FileSystem.hpp"
#include "config/Route.hpp"
#include "cgi/CgiHandler.hpp"
#include "core/Client.hpp"
#include <iostream>

namespace
{
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
}

void serveCgi(Client &connection, std::vector<Config> configs)
{
//	std::string body;
//	std::string content_type = "text/plain";
//	int resp_status = 200;
//
//	CgiHandler handler;
//	handler.setPythonInterpreter("/usr/bin/python3");
//	handler.setDocumentRoot(path);
//	connection.request.body = query;
//	connection.request.cgi = Python;
//	body = handler.handleRequest(connection, "cgi-bin/env.py");
//	std::cout << body << std::endl;
//
//	connection.out_buf += buildResponse(resp_status, body, connection.keep_alive, "Content-Type: text/html\r\n\r\n");
//
//	connection.state = Connection::WRITING;
//	serverutil::resetRequest(connection);
//	if (!connection.cgi_request || connection.cgi_script_path.empty())
//	{
//		respondError(connection, 500, "!connection.cgi_request || connection.cgi_script_path.empty()");
//		return;
//	}



	CgiHandler handler;

	// Determine interpreter based on file extension
	LOG_DBG << "HERE";
	std::string ext = getFileExtension(connection.request.query);
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
	const Config &cfg = serverutil::getConfigForConnection(connection, configs);
	handler.setDocumentRoot(cfg.getRoot());
	handler.setServerName(cfg.getServerName());
	handler.setServerPort(cfg.getPort());

	// Execute CGI script
	std::string script_name = handler.getScriptName(connection.request.query);
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

void Server::handleReadyRequest(Client &conn, std::vector<Config> configs)
{
    std::string uri = stripQuery(conn.request.target).first;
	conn.request.query = stripQuery(conn.request.target).second;
    if (uri.empty())
        uri = "/";

    const Config &cfg = (conn.config_index < _configs.size())
        ? _configs[conn.config_index]
        : _configs[0];
	LOG_DBG << "handleReadyRequest uri: " << uri;
    Location loc = matchLocation(cfg, uri);
	LOG_DBG << "handleReadyRequest matchLocation result: " << loc.getCgiBinPath();
	cfg.logDebug();

    if (!serverutil::isMethodAllowed(loc.getAllowedMethods(), conn.request.method))
    {
        respondError(conn, 405, "loc && !serverutil::isMethodAllowed(loc->getAllowedMethods(), conn.request.method)");
        return;
    }

    if (hasTraversal(uri))
    {
        respondError(conn, 403, "hasTraversal(uri)");
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

	conn.cgi_request = false;  // Reset first
	conn.location = &loc;       // Store location pointer
	if (loc.isCgiEnabled())
	{
		// Check if request targets a CGI script
		std::string ext = getFileExtension(conn.request.query);
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
				conn.cgi_request = true;

			}
			if (ext == ".py")
				conn.request.cgi = Python;
		}
		else
		{
			// Check against configured extensions
			for (size_t i = 0; i < cgi_exts.size(); ++i)
			{
				if (ext == cgi_exts[i])
				{
					conn.cgi_request = true;
					break;
				}
			}
		}

		if (conn.cgi_request)
		{
			conn.cgi_script_path = path;
			LOG_DBG << "LOG_DBG cgi_script_path " << path;
			conn.cgi_bin_path = !alias.empty() ? alias : loc.getCgiBinPath();
			// Extract PATH_INFO if there's additional path after script
			conn.cgi_path_info = ""; // Can be enhanced later
		}
	}

	if (handleUpload(conn, &loc, uri))
	{
		return;
	}
	if (handleDelete(conn, &loc, root, path, uri))
	{
		return;
	}

	std::vector<std::string> llc = conn.location->getCgiPath();
	for (size_t i = 0; i < llc.size(); i++) {
		LOG_DBG << "llc" << llc[i];
	}

	LOG_DBG << "request: " << path << " " << uri << " " << conn.request.query << " " << index << " " << autoindex;
	if (!conn.cgi_request)
		serveStatic(conn, path, uri, index, autoindex);
	else
		serveCgi(conn, configs);
}
