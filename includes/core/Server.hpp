#ifndef SERVER_HPP
#define SERVER_HPP

#include "core/Connection.hpp"

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

    void initListeningSockets();
    void buildPollFds(std::vector<struct pollfd> &pfds);
    void handlePollEvents(const std::vector<struct pollfd> &pfds);
    void handleListeningEvent(int fd);
    void handleClientRead(int fd);
    void handleClientWrite(int fd);
    void closeClient(int fd);
    void setNonBlocking(int fd);
    bool isListeningFd(int fd) const;

    std::vector<int> _listening_fds;
    std::set<int> _listening_set;
    std::map<int, Connection> _clients;
};

#endif // SERVER_HPP
