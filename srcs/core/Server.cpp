#include "../../includes/core/Server.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <iostream>

#include "../../includes/config/Config.hpp"
#include "../../includes/config/ConfigParser.hpp"

namespace
{
    const std::string kDefaultConfigPath = "conf/default.conf";
    const std::string kHttpResponse =
        "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";

    struct AddrInfoGuard
    {
        struct addrinfo *res;
        AddrInfoGuard() : res(0) {}
        ~AddrInfoGuard()
        {
            if (res)
                freeaddrinfo(res);
        }
    };

    struct FdGuard
    {
        int fd;
        FdGuard() : fd(-1) {}
        explicit FdGuard(int fd_) : fd(fd_) {}
        ~FdGuard()
        {
            if (fd >= 0)
                close(fd);
        }
        int release()
        {
            int tmp = fd;
            fd = -1;
            return tmp;
        }
    };
}

Server::Server()
{
    ConfigParser configParser;
    _configs = configParser.parseMultiple(&kDefaultConfigPath);
    initListeningSockets();
}

Server::Server(const std::string & config_path)
{
    ConfigParser configParser;
	_configs = configParser.parseMultiple(&config_path);
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

void printConfig(const Config& config)
{
	std::cout << "=== Server Configuration ===" << std::endl;
	std::cout << "Port: " << config.getPort() << std::endl;
	std::cout << "Server Name: " << config.getServerName() << std::endl;
	std::cout << "Host: " << config.getHost() << std::endl;
	std::cout << "Root: " << config.getRoot() << std::endl;
	std::cout << "Client Max Body Size: " << config.getClientMaxBodySize() << std::endl;
	std::cout << "Index: " << config.getIndex() << std::endl;

	ErrorPage ep = config.getErrorPage();
	if (ep.code != 0)
	{
		std::cout << "Error Page: " << ep.code << " -> " << ep.path << std::endl;
	}

	std::vector<Location> locations = config.getLocations();
	std::cout << "\nLocations (" << locations.size() << "):" << std::endl;

	for (size_t i = 0; i < locations.size(); i++)
	{
		std::cout << "\n  Location: " << locations[i].getPath() << std::endl;

		if (!locations[i].getRoot().empty())
		{
			std::cout << "    Root: " << locations[i].getRoot() << std::endl;
		}

		if (!locations[i].getIndex().empty())
		{
			std::cout << "    Index: " << locations[i].getIndex() << std::endl;
		}

		std::cout << "    Autoindex: " << (locations[i].getAutoindex() ? "on" : "off") << std::endl;

		std::vector<std::string> methods = locations[i].getAllowedMethods();
		if (!methods.empty())
		{
			std::cout << "    Allowed Methods: ";
			for (size_t j = 0; j < methods.size(); j++)
			{
				std::cout << methods[j];
				if (j < methods.size() - 1)
				{
					std::cout << ", ";
				}
			}
			std::cout << std::endl;
		}

		if (!locations[i].getRedirect().empty())
		{
			std::cout << "    Redirect: " << locations[i].getRedirect() << std::endl;
		}

		std::vector<std::string> cgi_paths = locations[i].getCgiPath();
		if (!cgi_paths.empty())
		{
			std::cout << "    CGI Paths: ";
			for (size_t j = 0; j < cgi_paths.size(); j++)
			{
				std::cout << cgi_paths[j];
				if (j < cgi_paths.size() - 1)
				{
					std::cout << ", ";
				}
			}
			std::cout << std::endl;
		}

		std::vector<std::string> cgi_exts = locations[i].getCgiExt();
		if (!cgi_exts.empty())
		{
			std::cout << "    CGI Extensions: ";
			for (size_t j = 0; j < cgi_exts.size(); j++)
			{
				std::cout << cgi_exts[j];
				if (j < cgi_exts.size() - 1)
				{
					std::cout << ", ";
				}
			}
			std::cout << std::endl;
		}

		if (!locations[i].getUploadPath().empty())
		{
			std::cout << "    Upload Path: " << locations[i].getUploadPath() << std::endl;
		}
	}

	std::cout << "\n============================\n" << std::endl;
}

void Server::initListeningSockets()
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	printConfig(this->_configs[0]);
	std::stringstream mainport;
	mainport << this->_configs[0].getPort();
	std::cout << this->_configs[0].getPort();

    AddrInfoGuard info;
    if (getaddrinfo(this->_configs[0].getHost().c_str(), mainport.str().c_str(), &hints, &info.res) != 0)
        throw std::runtime_error("getaddrinfo failed");

    for (struct addrinfo *p = info.res; p != 0; p = p->ai_next)
    {
        FdGuard sock(socket(p->ai_family, p->ai_socktype, p->ai_protocol));
        if (sock.fd < 0)
            continue;

        int opt = 1;
        if (setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            continue;

        setNonBlocking(sock.fd);

        if (bind(sock.fd, p->ai_addr, p->ai_addrlen) < 0)
            continue;

        if (listen(sock.fd, 128) < 0)
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

    char buffer[4096];
    ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
    if (n <= 0)
    {
        closeClient(fd);
        return;
    }

    Connection &conn = it->second;
    conn.in_buf.append(buffer, static_cast<size_t>(n));
    conn.out_buf = kHttpResponse;
    conn.state = Connection::WRITING;
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
