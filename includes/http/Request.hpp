#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <cstddef>
#include <map>
#include <string>

enum HttpMethod
{
    METHOD_GET,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_UNKNOWN
};

enum CGIMethod {
	PHP,
	Python,
	Shell,
	Static
};

class Request
{
public:
    Request();

    std::string method;
    HttpMethod method_enum;
    std::string target;
    std::string version;
    std::map<std::string, std::string> headers;
    size_t content_length;
    bool has_body;
    std::string body;
	CGIMethod cgi;
};

#endif
