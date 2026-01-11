#include "config/Config.hpp"
#include "utils/Logger.hpp"

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

std::vector<Location> Config::getLocations()
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

void Config::logDebug() const
{
	LOG_DBG << "\n=== Config Debug ==="
			<< "\n port=" << _port
			<< "\n server_name=" << _server_name
			<< "\n host=" << _host
			<< "\n root=" << _root
			<< "\n client_max_body_size=" << _client_max_body_size
			<< "\n index=" << _index
			<< "\n error_page.code=" << _error_page.code
			<< "\n error_page.path=" << _error_page.path
			<< "\n locations.count=" << _locations.size();

	for (size_t i = 0; i < _locations.size(); ++i)
	{
		const Location& loc = _locations[i];
		LOG_DBG << "\n  Location[" << i << "]:"
				<< "\n path=" << loc.getPath()
				<< "\n root=" << loc.getRoot()
				<< "\n alias=" << loc.getAlias()
				<< "\n index=" << loc.getIndex()
				<< "\n autoindex=" << (loc.getAutoindex() ? "true" : "false")
				<< "\n redirect=" << loc.getRedirect()
				<< "\n upload_path=" << loc.getUploadPath()
				<< "\n cgi_enabled=" << (loc.isCgiEnabled() ? "true" : "false")
				<< "\n cgi_bin_path=" << loc.getCgiBinPath();
	}
}