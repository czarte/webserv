#include "core/Server.hpp"
#include "core/ServerInternal.hpp"
#include "core/Fd.hpp"
#include "config/ConfigParser.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <map>
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

	std::map<std::string, std::vector<Config> > groups;
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		std::ostringstream key;
		key << _configs[i].getHost() << ":" << _configs[i].getPort();
		groups[key.str()].push_back(_configs[i]);
	}

	for (std::map<std::string, std::vector<Config> >::iterator it = groups.begin();
		 it != groups.end(); ++it)
	{
		_workers.push_back(new Worker(it->second));
	}
}

Server::~Server()
{
	for (size_t i = 0; i < _workers.size(); ++i)
		delete _workers[i];
}


