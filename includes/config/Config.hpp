#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

class Location;

struct ErrorPage
{
    int code;
    std::string path;
    
    ErrorPage();
};

class Config
{
public:
    Config();
    Config(int port, const std::string& server_name, const std::string& host,
           const std::string& root, int client_max_body_size, const std::string& index);
    ~Config();
    
    // Getters
    int getPort() const;
    std::string getServerName() const;
    std::string getHost() const;
    std::string getRoot() const;
    int getClientMaxBodySize() const;
    std::string getIndex() const;
    ErrorPage getErrorPage() const;
    std::vector<Location> getLocations() const;
    
    // Setters
    void setPort(int port);
    void setServerName(const std::string& server_name);
    void setHost(const std::string& host);
    void setRoot(const std::string& root);
    void setClientMaxBodySize(int size);
    void setIndex(const std::string& index);
    void setErrorPage(const ErrorPage& error_page);
    void addLocation(const Location& location);

private:
    int _port;
    std::string _server_name;
    std::string _host;
    std::string _root;
    int _client_max_body_size;
    std::string _index;
    ErrorPage _error_page;
    std::vector<Location> _locations;
};

#endif