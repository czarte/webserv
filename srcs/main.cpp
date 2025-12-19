
#include "core/Server.hpp"

#include <exception>
#include <iostream>
#include <signal.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    try
    {
        signal(SIGPIPE, SIG_IGN);
        Server server;
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
