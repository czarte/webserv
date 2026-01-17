#ifndef SERVER_HPP
#define SERVER_HPP

#include "core/Worker.hpp"
#include "config/Config.hpp"
#include <vector>

#include <netdb.h>
#include <poll.h>

class Server
{
public:
	Server();
	~Server();

	void run();

private:
	Server(const Server &);
	Server &operator=(const Server &);

	void buildPollFds(std::vector<struct pollfd> &pfds);
	void handlePollEvents(const std::vector<struct pollfd> &pfds);

	std::vector<Worker*> _workers;
	std::vector<Config> _configs;
};

#endif // SERVER_HPP
