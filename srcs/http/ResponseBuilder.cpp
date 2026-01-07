#include "http/ResponseBuilder.hpp"

#include <cctype>
#include <sstream>

namespace
{
std::string toLower(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

const char *statusMessage(int status)
{
    switch (status)
    {
    case 201: return "Created";
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    default:  return "Error";
    }
}
}

std::string contentTypeForPath(const std::string &path)
{
    std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "text/plain";
    std::string ext = toLower(path.substr(dot + 1));
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "txt")
        return "text/plain";
    return "application/octet-stream";
}

std::string buildResponse(int status, const std::string &body, bool keep_alive,
                          const std::string &content_type)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << status << " " << statusMessage(status) << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Content-Type: " << content_type << "\r\n";
    out << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";
    out << "\r\n";
    out << body;
    return out.str();
}

std::string buildErrorResponse(int status, bool keep_alive)
{
    std::string body = statusMessage(status);
    body += "\n";
    return buildResponse(status, body, keep_alive, "text/plain");
}
