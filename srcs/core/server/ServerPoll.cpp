#include "core/Server.hpp"
#include "core/ServerInternal.hpp"

void Server::buildPollFds(std::vector<struct pollfd> &pfds)
{
	pfds.clear();

	// Each Worker adds its listening_fd and client fds
	for (size_t i = 0; i < _workers.size(); ++i)
	{
		_workers[i]->addToPollFds(pfds);
	}
}

void Server::handlePollEvents(const std::vector<struct pollfd> &pfds)
{
	for (size_t i = 0; i < pfds.size(); ++i)
	{
		const struct pollfd &pfd = pfds[i];

		// Find which Worker owns this fd
		for (size_t w = 0; w < _workers.size(); ++w)
		{
			Worker *worker = _workers[w];

			if (worker->isListeningFd(pfd.fd) || worker->hasClient(pfd.fd))
			{
				worker->handlePollEvent(pfd);
				break;
			}
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

		// Check timeouts in all workers
		for (size_t i = 0; i < _workers.size(); ++i)
		{
			_workers[i]->checkTimeouts();
		}
	}
}
