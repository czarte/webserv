#include "config/Location.hpp"

Location::Location()
    : _path(),
      _root(),
      _index(),
      _autoindex(false),
      _allowed_methods(),
      _redirect(),
      _cgi_path(),
      _cgi_ext(),
      _upload_path()
{
}

Location::Location(const std::string& path)
    : _path(path),
      _root(),
      _index(),
      _autoindex(false),
      _allowed_methods(),
      _redirect(),
      _cgi_path(),
      _cgi_ext(),
      _upload_path()
{
}

Location::~Location()
{
}

// Getters
std::string Location::getPath() const
{
    return _path;
}

std::string Location::getRoot() const
{
    return _root;
}

std::string Location::getIndex() const
{
    return _index;
}

bool Location::getAutoindex() const
{
    return _autoindex;
}

std::vector<std::string> Location::getAllowedMethods() const
{
    return _allowed_methods;
}

std::string Location::getRedirect() const
{
    return _redirect;
}

std::vector<std::string> Location::getCgiPath() const
{
    return _cgi_path;
}

std::vector<std::string> Location::getCgiExt() const
{
    return _cgi_ext;
}

std::string Location::getUploadPath() const
{
    return _upload_path;
}

// Setters
void Location::setPath(const std::string& path)
{
    _path = path;
}

void Location::setRoot(const std::string& root)
{
    _root = root;
}

void Location::setIndex(const std::string& index)
{
    _index = index;
}

void Location::setAutoindex(bool autoindex)
{
    _autoindex = autoindex;
}

void Location::addAllowedMethod(const std::string& method)
{
    _allowed_methods.push_back(method);
}

void Location::setRedirect(const std::string& redirect)
{
    _redirect = redirect;
}

void Location::addCgiPath(const std::string& cgi_path)
{
    _cgi_path.push_back(cgi_path);
}

void Location::addCgiExt(const std::string& cgi_ext)
{
    _cgi_ext.push_back(cgi_ext);
}

void Location::setUploadPath(const std::string& upload_path)
{
    _upload_path = upload_path;
}