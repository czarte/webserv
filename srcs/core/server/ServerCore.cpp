#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "core/Fd.hpp"
#include "config/ConfigParser.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "io/NonBlocking.hpp"

namespace serverutil
{
	const char *kDefaultConfigPath = "conf/default.conf";
}

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
}

Server::Server()
{
    ConfigParser parser;
    std::string path(serverutil::kDefaultConfigPath);
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

		int client_fd = sock.get();
		if (makeNonBlocking(client_fd) == -1)
		{
			close(client_fd);
			continue;  // Skip this client but continue accepting others
		}

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
