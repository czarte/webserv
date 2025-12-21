#include "core/Server.hpp"
#include "core/Fd.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>



#include <cerrno>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
    const char *kDefaultHost = "127.0.0.1";
    const char *kDefaultPort = "8080";

    const std::string kHttpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "OK";

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
    initListeningSockets();
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

void Server::initListeningSockets()
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    AddrInfoGuard info;
    if (getaddrinfo(kDefaultHost, kDefaultPort, &hints, &info.res) != 0)
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

        _clients.insert(std::make_pair(client_fd, Connection(client_fd)));
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
        conn.out_buf = kHttpResponse;
        conn.state = Connection::WRITING;
    }
}






void Server::handleClientWrite(int fd)
{
    std::map<int, Connection>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;

    Connection &conn = it->second;
    ssize_t n = send(fd, conn.out_buf.c_str(), conn.out_buf.size(), 0);
    if (n <= 0)
    {
        closeClient(fd);
        return;
    }

    conn.out_buf.erase(0, static_cast<size_t>(n));
    if (conn.out_buf.empty())
        closeClient(fd);
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
    }
}
