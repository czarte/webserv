#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "http/Parser.hpp"
#include "http/ResponseBuilder.hpp"

#include <cerrno>
#include <cctype>

#include <sys/socket.h>
#include <unistd.h>
#include "io/NonBlocking.hpp"

namespace
{
	const size_t kMaxRequestLine = 8192;
	const size_t kMaxHeadersSize = 65536;

	int parseChunkSize(const std::string &line, size_t &out)
	{
		std::string::size_type semi = line.find(';');
		std::string size_part = (semi == std::string::npos) ? line : line.substr(0, semi);
		size_part = serverutil::trim(size_part);
		if (size_part.empty())
			return 400;
		size_t value = 0;
		for (size_t i = 0; i < size_part.size(); ++i)
		{
			char c = size_part[i];
			if (!std::isxdigit(static_cast<unsigned char>(c)))
				return 400;
			value *= 16;
			if (c >= '0' && c <= '9')
				value += static_cast<size_t>(c - '0');
			else if (c >= 'a' && c <= 'f')
				value += static_cast<size_t>(10 + c - 'a');
			else if (c >= 'A' && c <= 'F')
				value += static_cast<size_t>(10 + c - 'A');
		}
		out = value;
		return 0;
	}

	int consumeChunked(Client &conn, std::string &err)
	{
		for (;;)
		{
			if (conn.chunk_reading_trailer)
			{
				if (conn.in_buf.size() >= 2 && conn.in_buf.compare(0, 2, "\r\n") == 0)
				{
					conn.in_buf.erase(0, 2);
					conn.chunk_reading_trailer = false;
					conn.chunked = false;
					return 1;
				}

				std::string::size_type end = conn.in_buf.find("\r\n\r\n");
				if (end == std::string::npos)
					return 0;
				std::string trailer_block = conn.in_buf.substr(0, end + 4);
				conn.in_buf.erase(0, end + 4);

				std::string::size_type pos = 0;
				while (pos < trailer_block.size())
				{
					std::string::size_type next = trailer_block.find("\r\n", pos);
					if (next == std::string::npos)
						break;
					if (next == pos)
						break;
					std::string line = trailer_block.substr(pos, next - pos);
					pos = next + 2;

					std::string::size_type colon = line.find(':');
					if (colon == std::string::npos)
					{
						err = "bad trailer header";
						return -1;
					}
					std::string key = serverutil::toLower(serverutil::trim(line.substr(0, colon)));
					std::string val = serverutil::trim(line.substr(colon + 1));
					if (key.empty())
					{
						err = "empty trailer key";
						return -1;
					}
					conn.request.headers[key] = val;
				}

				conn.chunk_reading_trailer = false;
				conn.chunked = false;
				return 1;
			}

			if (conn.chunk_bytes_remaining == 0)
			{
				std::string::size_type line_end = conn.in_buf.find("\r\n");
				if (line_end == std::string::npos)
					return 0;
				std::string line = conn.in_buf.substr(0, line_end);
				conn.in_buf.erase(0, line_end + 2);
				size_t size = 0;
				if (parseChunkSize(line, size) != 0)
				{
					err = "bad chunk size";
					return -1;
				}
				if (size == 0)
				{
					conn.chunk_reading_trailer = true;
					continue;
				}
				conn.chunk_bytes_remaining = size;
			}

			if (conn.in_buf.size() < conn.chunk_bytes_remaining + 2)
				return 0;
			conn.request.body.append(conn.in_buf, 0, conn.chunk_bytes_remaining);
			conn.in_buf.erase(0, conn.chunk_bytes_remaining);
			if (conn.in_buf.size() < 2 || conn.in_buf.compare(0, 2, "\r\n") != 0)
			{
				err = "missing chunk CRLF";
				return -1;
			}
			conn.in_buf.erase(0, 2);
			conn.chunk_bytes_remaining = 0;
		}
	}
}

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
                if (conn.chunked)
                {
                    std::string err;
                    int rc = consumeChunked(conn, err);
                    if (rc == 0)
                        break;
                    if (rc < 0)
                    {
                        conn.keep_alive = false;
                        conn.out_buf += buildErrorResponse(400, false, err);
                        conn.state = Client::WRITING;
                        serverutil::resetRequest(conn);
                        break;
                    }
                    conn.state = Client::READING_HEADERS;
                    handleReadyRequest(conn, _configs);
                    if (conn.state == Client::WRITING)
                        break;
                    continue;
                }

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

            std::string::size_type line_end = conn.in_buf.find("\r\n");
            if (line_end == std::string::npos)
            {
                if (conn.in_buf.size() > kMaxRequestLine)
                {
                    conn.keep_alive = false;
                    conn.out_buf += buildErrorResponse(414, false, "Request line too long");
                    conn.state = Client::WRITING;
                    break;
                }
            }
            else if (line_end > kMaxRequestLine)
            {
                conn.keep_alive = false;
                conn.out_buf += buildErrorResponse(414, false, "Request line too long");
                conn.state = Client::WRITING;
                break;
            }

            std::string::size_type header_end = conn.in_buf.find("\r\n\r\n");
            if (header_end == std::string::npos)
            {
                if (conn.in_buf.size() > kMaxHeadersSize)
                {
                    conn.keep_alive = false;
                    conn.out_buf += buildErrorResponse(431, false, "Headers too large");
                    conn.state = Client::WRITING;
                    break;
                }
            }
            else if (header_end > kMaxHeadersSize)
            {
                conn.keep_alive = false;
                conn.out_buf += buildErrorResponse(431, false, "Headers too large");
                conn.state = Client::WRITING;
                break;
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

            std::map<std::string, std::string>::iterator it_te =
                conn.request.headers.find("transfer-encoding");
            if (it_te != conn.request.headers.end()
                && serverutil::toLower(it_te->second) == "chunked")
            {
                conn.chunked = true;
                conn.chunk_bytes_remaining = 0;
                conn.chunk_reading_trailer = false;
                conn.state = Client::READING_BODY;
                continue;
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
