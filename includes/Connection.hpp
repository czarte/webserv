#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>

struct Connection
{
    enum State
    {
        READING,
        WRITING
    };

    int fd;
    std::string in_buf;
    std::string out_buf;
    State state;

    Connection() : fd(-1), in_buf(), out_buf(), state(READING) {}
    explicit Connection(int f) : fd(f), in_buf(), out_buf(), state(READING) {}
};

#endif // CONNECTION_HPP
