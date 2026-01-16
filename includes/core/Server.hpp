#ifndef SERVER_HPP
#define SERVER_HPP

#include "core/Client.hpp"
#include "config/Config.hpp"

#include <map>
#include <set>
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

    void initListeningSockets(const Config &config, size_t config_index);
    void buildPollFds(std::vector<struct pollfd> &pfds);
    void handlePollEvents(const std::vector<struct pollfd> &pfds);
    void handleListeningEvent(int fd);
    void handleClientRead(int fd);
    void handleClientWrite(int fd);
    void handleReadyRequest(Client &conn, std::vector<Config> _configs);
    void closeClient(int fd);
    bool isListeningFd(int fd) const;

    std::vector<int> _listening_fds;
    std::set<int> _listening_set;
    std::map<int, size_t> _listen_config;
    std::map<int, Client> _clients;
    std::vector<Config> _configs;
};

#endif // SERVER_HPP
