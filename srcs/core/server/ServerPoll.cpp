#include "core/Server.hpp"
#include "core/ServerInternal.hpp"

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

    for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
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

        std::map<int, Client>::iterator client_it = _clients.find(it->fd);
        if (client_it == _clients.end())
            continue;

        if (it->revents & POLLIN)
        {
            if (client_it->second.state == Client::READING_HEADERS
                || client_it->second.state == Client::READING_BODY)
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
}
