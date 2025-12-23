#include "core/Server.hpp"
#include "core/Fd.hpp"
#include "http/Parser.hpp"
#include "http/ResponseBuilder.hpp"
#include "utils/Path.hpp"
#include "utils/Time.hpp"
#include "io/FileSystem.hpp"
#include "config/ConfigParser.hpp"
#include "config/Route.hpp"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <cerrno>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
    const char *kDefaultConfigPath = "conf/default.conf";
    const size_t kHeaderTimeoutMs = 5000;
    const size_t kIdleTimeoutMs = 15000;

    // const std::string kHttpResponse =
    //     "HTTP/1.1 200 OK \r\n"
    //     "Content-Length: 4\r\n"
    //     "Connection: close\r\n"
    //     "\r\n"
    //     "OK!!";

    struct AddrInfoGuard
    {
        addrinfo *res;
        AddrInfoGuard() : res(0) {}
        ~AddrInfoGuard() { if (res) freeaddrinfo(res); }

    private:
        AddrInfoGuard(const AddrInfoGuard &);
        AddrInfoGuard &operator=(const AddrInfoGuard &);
    };

}


Server::Server()
{
    ConfigParser parser;
    std::string path(kDefaultConfigPath);
    _configs = parser.parseMultiple(&path);
    if (_configs.empty())
        throw std::runtime_error("No server configurations loaded");
    for (size_t i = 0; i < _configs.size(); ++i)
        initListeningSockets(_configs[i], i);
}

Server::~Server()
{
    for (std::map<int, Connection>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        close(it->first);
    for (std::vector<int>::iterator it = _listening_fds.begin(); it != _listening_fds.end(); ++it)
        close(*it);
}

void Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::initListeningSockets(const Config &config, size_t config_index)
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    AddrInfoGuard info;
    std::stringstream port;
    port << config.getPort();
    if (getaddrinfo(config.getHost().c_str(), port.str().c_str(), &hints, &info.res) != 0)
        throw std::runtime_error("getaddrinfo failed");

    for (struct addrinfo *p = info.res; p != 0; p = p->ai_next)
    {
        Fd sock(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
        if (sock.get() < 0)
            continue;

        int opt = 1;
        if (setsockopt(sock.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            continue;

        setNonBlocking(sock.get());

        if (bind(sock.get(), p->ai_addr, p->ai_addrlen) < 0)
            continue;

        if (listen(sock.get(), 128) < 0)
            continue;

        int listen_fd = sock.release();
        _listening_fds.push_back(listen_fd);
        _listening_set.insert(listen_fd);
        _listen_config.insert(std::make_pair(listen_fd, config_index));
    }

    if (_listening_fds.empty())
        throw std::runtime_error("No listening sockets available");
}

bool Server::isListeningFd(int fd) const
{
    return _listening_set.find(fd) != _listening_set.end();
}

void Server::buildPollFds(std::vector<struct pollfd> &pfds)
{
    pfds.clear();
    pfds.reserve(_listening_fds.size() + _clients.size());
    for (std::vector<int>::const_iterator it = _listening_fds.begin(); it != _listening_fds.end(); ++it)
    {
        struct pollfd pfd;
        pfd.fd = *it;
        pfd.events = POLLIN;
        pfd.revents = 0;
        pfds.push_back(pfd);
    }

    for (std::map<int, Connection>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        struct pollfd pfd;
        pfd.fd = it->first;
        // This keeps the "I/O gated by poll revents" rule and supports simultaneous read/write readiness.
        short ev = POLLIN;
        if (!it->second.out_buf.empty())
            ev |= POLLOUT;
        pfd.events = ev;
        pfd.revents = 0;
        pfds.push_back(pfd);
    }
}

void Server::handleListeningEvent(int fd)
{
    while (true)
    {
        int client_fd = accept(fd, 0, 0);
        if (client_fd < 0)
            break;
        setNonBlocking(client_fd);
        Connection conn(client_fd);
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
    std::map<int, Connection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Connection &conn = it->second;

    char buffer[4096];

    for (;;)
    {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

        if (n > 0)
        {
            conn.in_buf.append(buffer, static_cast<size_t>(n));
            conn.last_activity_ms=now_ms();
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
                conn.request.headers.find("Connection");
            if (it != conn.request.headers.end())
            {
                if (it->second == "close")
                    conn.keep_alive = false;
                else if (it->second == "keep-alive")
                    conn.keep_alive = true;
            }

            if (result == Parser::PARSE_ERROR || status != 200)
            {
                conn.keep_alive = false;
                conn.out_buf += buildErrorResponse(status, false);
                conn.state = Connection::WRITING;
                break;
            }

            std::string uri = stripQuery(conn.request.target);
            if (uri.empty())
                uri = "/";
            if (hasTraversal(uri))
            {
                conn.out_buf += buildErrorResponse(403, conn.keep_alive);
                conn.state = Connection::WRITING;
                break;
            }

            const Config &cfg = (conn.config_index < _configs.size())
                ? _configs[conn.config_index]
                : _configs[0];
            const Location *loc = matchLocation(cfg, uri);
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
                conn.out_buf += buildErrorResponse(resp_status, conn.keep_alive);
            else
                conn.out_buf += buildResponse(200, body, conn.keep_alive, content_type);

            conn.state = Connection::WRITING;
            conn.request = Request();
        }
    }
}






void Server::handleClientWrite(int fd)
{
    std::map<int, Connection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Connection &conn = it->second;
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
    conn.last_activity_ms=now_ms();
    if (conn.out_buf.empty() )
    {
        if(conn.keep_alive)
        {
            conn.header_start_ms = now_ms();
            conn.state = Connection::READING;
        }
        else 
            closeClient(fd);
    }
}

void Server::handlePollEvents(const std::vector<struct pollfd> &pfds)
{
    for (std::vector<struct pollfd>::const_iterator it = pfds.begin(); it != pfds.end(); ++it)
    {
        if (it->revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            if (_clients.find(it->fd) != _clients.end())
                closeClient(it->fd);
            continue;
        }

        if (isListeningFd(it->fd))
        {
            if (it->revents & POLLIN)
                handleListeningEvent(it->fd);
            continue;
        }

        std::map<int, Connection>::iterator client_it = _clients.find(it->fd);
        if (client_it == _clients.end())
            continue;

        if (it->revents & POLLIN)
        {
            if (client_it->second.state == Connection::READING)
                handleClientRead(it->fd);
        }

        client_it = _clients.find(it->fd);
        if (client_it == _clients.end())
            continue;

        if (it->revents & POLLOUT)
        {
            if (!client_it->second.out_buf.empty())
                handleClientWrite(it->fd);
        }
    }
}

void Server::run()
{
    std::vector<struct pollfd> pfds;
    while (true)
    {
        buildPollFds(pfds);
        struct pollfd *data = pfds.empty() ? 0 : &pfds[0];
        int ret = poll(data, pfds.size(), 1000);
        if (ret <= 0)
            continue;
        handlePollEvents(pfds);
        for (std::map<int, Connection>::iterator it = _clients.begin(); it != _clients.end(); )
        {
            Connection &conn = it->second;
            if (conn.state == Connection::READING
                && elapsed_ms(conn.header_start_ms) > kHeaderTimeoutMs)
            {
                int fd = it->first;
                ++it;
                closeClient(fd);
                continue;
            }
            ++it;
        }
        for (std::map<int, Connection>::iterator it = _clients.begin(); it != _clients.end(); )
        {
            if (elapsed_ms(it->second.last_activity_ms) > kIdleTimeoutMs)
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
}
