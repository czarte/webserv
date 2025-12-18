// #include <iostream>
// #include <unistd.h>

// int main() {
// 	std::cout << "WebServ started " << std::endl;
// 	while (true) {
// 		sleep(1000);
// 	}
// 	return 0;
// }


#include "Server.hpp"

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
