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
                else if (index.find('.') == std::string::npos)
                {
                    std::string html_path = idx_path + ".html";
                    if (isFile(html_path))
                    {
                        body = readFile(html_path, ok);
                        content_type = contentTypeForPath(html_path);
                    }
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
	CgiHandler handler;

	// Determine interpreter based on file extension
	std::string ext = getFileExtension(connection.request.query);
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
