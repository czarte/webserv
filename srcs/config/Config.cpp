#include "config/Config.hpp"

ErrorPage::ErrorPage() : code(0), path()
{
}

Config::Config()
    : _port(0),
      _server_name(),
      _host(),
      _root(),
      _client_max_body_size(0),
      _index(),
      _error_page(),
      _locations()
{
}

Config::Config(int port, const std::string& server_name, const std::string& host,
               const std::string& root, int client_max_body_size, const std::string& index)
    : _port(port),
      _server_name(server_name),
      _host(host),
      _root(root),
      _client_max_body_size(client_max_body_size),
      _index(index),
      _error_page(),
      _locations()
{
}

Config::~Config()
{
}

// Getters
int Config::getPort() const
{
    return _port;
}

std::string Config::getServerName() const
{
    return _server_name;
}

std::string Config::getHost() const
{
    return _host;
}

std::string Config::getRoot() const
{
    return _root;
}

int Config::getClientMaxBodySize() const
{
    return _client_max_body_size;
}

std::string Config::getIndex() const
{
    return _index;
}

ErrorPage Config::getErrorPage() const
{
    return _error_page;
}

std::vector<Location> Config::getLocations() const
{
    return _locations;
}

// Setters
void Config::setPort(int port)
{
    _port = port;
}

void Config::setServerName(const std::string& server_name)
{
    _server_name = server_name;
}

void Config::setHost(const std::string& host)
{
    _host = host;
}

void Config::setRoot(const std::string& root)
{
    _root = root;
}

void Config::setClientMaxBodySize(int size)
{
    _client_max_body_size = size;
}

void Config::setIndex(const std::string& index)
{
    _index = index;
}

void Config::setErrorPage(const ErrorPage& error_page)
{
    _error_page = error_page;
}

void Config::addLocation(const Location& location)
{
    _locations.push_back(location);
}