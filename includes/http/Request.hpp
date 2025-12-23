#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <map>
#include <string>

class Request
{
public:
    Request();

    std::string method;
    std::string target;
    std::string version;
    std::map<std::string, std::string> headers;
};

#endif
