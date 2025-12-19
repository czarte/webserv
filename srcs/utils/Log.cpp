#include "utils/Log.hpp"
#include <iostream>

void logutil::info(const char *msg)
{
    std::cout << msg << std::endl;
}

void logutil::error(const char *msg)
{
    std::cerr << msg << std::endl;
}
