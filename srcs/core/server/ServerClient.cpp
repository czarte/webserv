#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "http/Parser.hpp"
#include "http/ResponseBuilder.hpp"

#include <cerrno>

#include <sys/socket.h>
#include <unistd.h>
#include "io/NonBlocking.hpp"

void Server::handleListeningEvent(int fd)
{
    while (true)
    {
        int client_fd = accept(fd, 0, 0);
        if (client_fd < 0)
            break;
		if (makeNonBlocking(client_fd) == -1)
		{
			close(client_fd);
			continue;  // Skip this client but continue accepting others
		}
        Client conn(client_fd);
        conn.last_activity_ms = now_ms();
        conn.header_start_ms = conn.last_activity_ms;
        std::map<int, size_t>::const_iterator cfg = _listen_config.find(fd);
        if (cfg != _listen_config.end())
            conn.config_index = cfg->second;
        _clients.insert(std::make_pair(client_fd, conn));
    }
}

void Server::closeClient(int fd)
{
    close(fd);
    _clients.erase(fd);
}

void Server::handleClientRead(int fd)
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
                handleReadyRequest(conn, _configs);
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
                if (err.empty())
                    conn.out_buf += buildErrorResponse(status, false, "Parser error");
                else
                    conn.out_buf += buildErrorResponse(status, false, "Parser error: " + err);
                conn.state = Client::WRITING;
                break;
            }

            const Config &cfg = (conn.config_index < _configs.size())
                ? _configs[conn.config_index]
                : _configs[0];
            if (conn.request.has_body
                && cfg.getClientMaxBodySize() > 0
                && conn.request.content_length > static_cast<size_t>(cfg.getClientMaxBodySize()))
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

            handleReadyRequest(conn, _configs);
            if (conn.state == Client::WRITING)
                break;
        }
    }
}

void Server::handleClientWrite(int fd)
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
