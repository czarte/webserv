#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>

class WebservError : public std::runtime_error
{
public:
    explicit WebservError(const char *msg) : std::runtime_error(msg) {}
};

#endif // ERROR_HPP
