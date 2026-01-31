#include "config/Config.hpp"
#include "utils/Logger.hpp"

#include <sstream>

ErrorPage::ErrorPage() : code(0), path()
{
}

Config::Config()
    : _port(0),
      _server_name(),
      _server_names(),
      _host(),
      _root(),
      _cgi_bin_path(),
      _client_max_body_size(0),
      _index(),
      _error_page(),
      _locations(),
      _session_enabled(true)
{
}

Config::Config(int port, const std::string& server_name, const std::string& host,
               const std::string& root, int client_max_body_size, const std::string& index)
    : _port(port),
      _server_name(server_name),
      _server_names(),
      _host(host),
      _root(root),
      _cgi_bin_path(),
      _client_max_body_size(client_max_body_size),
      _index(index),
      _error_page(),
      _locations(),
      _session_enabled(true)
{
    if (!server_name.empty())
        _server_names.push_back(server_name);
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

std::vector<std::string> Config::getServerNames() const
{
    return _server_names;
}

std::string Config::getHost() const
{
    return _host;
}

std::string Config::getRoot() const
{
    return _root;
}

std::string Config::getCgiBinPath() const
{
    return _cgi_bin_path;
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

const std::vector<Location>& Config::getLocations() const
{
    return _locations;
}

std::vector<Location> Config::getLocations()
{
	return _locations;
}

bool Config::getSessionEnabled() const
{
    return _session_enabled;
}

// Setters
void Config::setPort(int port)
{
    _port = port;
}

void Config::setServerName(const std::string& server_name)
{
    _server_name = server_name;
    _server_names.clear();
    if (!server_name.empty())
        _server_names.push_back(server_name);
}

void Config::addServerName(const std::string& server_name)
{
    if (server_name.empty())
        return;
    if (_server_name.empty())
        _server_name = server_name;
    for (size_t i = 0; i < _server_names.size(); ++i)
    {
        if (_server_names[i] == server_name)
            return;
    }
    _server_names.push_back(server_name);
}

void Config::setHost(const std::string& host)
{
    _host = host;
}

void Config::setRoot(const std::string& root)
{
    _root = root;
}

void Config::setCgiBinPath(const std::string& path)
{
    _cgi_bin_path = path;
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

void Config::setSessionEnabled(bool enabled)
{
    _session_enabled = enabled;
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
			<< "\n session_enabled=" << (_session_enabled ? "true" : "false")
			<< "\n locations.count=" << _locations.size();

	if (!_server_names.empty())
	{
		std::ostringstream names;
		for (size_t i = 0; i < _server_names.size(); ++i)
		{
			if (i > 0)
				names << ", ";
			names << _server_names[i];
		}
		LOG_DBG << "\n server_names=" << names.str();
	}

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
