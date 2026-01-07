#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "http/ResponseBuilder.hpp"
#include "utils/Path.hpp"
#include "io/FileSystem.hpp"
#include "config/Route.hpp"

namespace
{
    void respondError(Connection &conn, int status)
    {
        conn.out_buf += buildErrorResponse(status, conn.keep_alive);
        conn.state = Connection::WRITING;
        serverutil::resetRequest(conn);
    }

    void respondText(Connection &conn, int status, const char *body)
    {
        conn.out_buf += buildResponse(status, body, conn.keep_alive, "text/plain");
        conn.state = Connection::WRITING;
        serverutil::resetRequest(conn);
    }

    bool handleUpload(Connection &conn, const Location *loc, const std::string &uri)
    {
        if (conn.request.method_enum != METHOD_POST && conn.request.method_enum != METHOD_PUT)
            return false;

        std::string upload_root;
        if (loc)
            upload_root = loc->getUploadPath();
        if (upload_root.empty())
        {
            respondError(conn, 403);
            return true;
        }
        if (!isDirectory(upload_root))
        {
            respondError(conn, 500);
            return true;
        }
        std::string name = serverutil::lastPathSegment(uri);
        if (name.empty())
            name = serverutil::buildUploadName();
        std::string out_path = joinPath(upload_root, name);
        if (!serverutil::isPathWithinRoot(upload_root, out_path))
        {
            respondError(conn, 403);
            return true;
        }
        if (!writeFile(out_path, conn.request.body))
        {
            respondError(conn, 500);
            return true;
        }
        respondText(conn, 201, "Created\n");
        return true;
    }

    bool handleDelete(Connection &conn, const Location *loc, const std::string &root,
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
            respondError(conn, 403);
            return true;
        }
        if (use_upload_root && !isDirectory(delete_root))
        {
            respondError(conn, 500);
            return true;
        }

        std::string delete_path;
        if (use_upload_root)
        {
            std::string name = serverutil::lastPathSegment(uri);
            if (name.empty())
            {
                respondError(conn, 403);
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
            respondError(conn, 403);
            return true;
        }

        if (isDirectory(delete_path))
        {
            respondError(conn, 403);
            return true;
        }
        if (!isFile(delete_path))
        {
            respondError(conn, 404);
            return true;
        }
        if (!deleteFile(delete_path))
        {
            respondError(conn, 500);
            return true;
        }
        respondText(conn, 200, "OK\n");
        return true;
    }

    void serveStatic(Connection &conn, const std::string &path, const std::string &uri,
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
            respondError(conn, resp_status);
        else
            conn.out_buf += buildResponse(200, body, conn.keep_alive, content_type);

        conn.state = Connection::WRITING;
        serverutil::resetRequest(conn);
    }
}

void Server::handleReadyRequest(Connection &conn)
{
    std::string uri = stripQuery(conn.request.target);
    if (uri.empty())
        uri = "/";

    const Config &cfg = (conn.config_index < _configs.size())
        ? _configs[conn.config_index]
        : _configs[0];
    const Location *loc = matchLocation(cfg, uri);

    if (loc && !serverutil::isMethodAllowed(loc->getAllowedMethods(), conn.request.method))
    {
        respondError(conn, 405);
        return;
    }

    if (hasTraversal(uri))
    {
        respondError(conn, 403);
        return;
    }
    std::string root = cfg.getRoot();
    std::string index = cfg.getIndex();
    bool autoindex = false;
    if (loc)
    {
        if (!loc->getRoot().empty())
            root = loc->getRoot();
        if (!loc->getIndex().empty())
            index = loc->getIndex();
        autoindex = loc->getAutoindex();
    }

    std::string path = joinPath(root, uri);

    if (handleUpload(conn, loc, uri))
        return;
    if (handleDelete(conn, loc, root, path, uri))
        return;

    if (!serverutil::isPathWithinRoot(root, path))
    {
        respondError(conn, 403);
        return;
    }

    serveStatic(conn, path, uri, index, autoindex);
}
