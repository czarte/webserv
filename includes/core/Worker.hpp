#ifndef WEBSERV_WORKER_HPP
#define WEBSERV_WORKER_HPP

#include "core/Client.hpp"
#include "config/Config.hpp"
#include <map>
#include <vector>
#include <poll.h>

class Worker {
public:
	explicit Worker(const Config &config);
	~Worker();

	// Socket management
	int getListeningFd() const { return _listening_fd; }
	bool isListeningFd(int fd) const { return fd == _listening_fd; }

	// Client management
	void acceptNewClient();
	void handleClientRead(int fd);
	void handleClientWrite(int fd);
	void closeClient(int fd);
	bool hasClient(int fd) const { return _clients.find(fd) != _clients.end(); }

	// Request processing
	void handleReadyRequest(Client &conn);

	// Poll support
	void addToPollFds(std::vector<struct pollfd> &pfds) const;
	void handlePollEvent(const struct pollfd &pfd);

	// Timeout checks
	void checkTimeouts();

	// Accessors
	const std::map<int, Client>& getClients() const { return _clients; }

private:
	Worker(const Worker &);
	Worker &operator=(const Worker &);

	void initListeningSocket();

	int _listening_fd;
	std::map<int, Client> _clients;
	Config _config;
};

#endif //WEBSERV_WORKER_HPP
