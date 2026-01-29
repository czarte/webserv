#include "config/ConfigParser.hpp"
#include "utils/Logger.hpp"
#include <stdexcept>
#include <cstdlib>
#include <cctype>
#include <iostream>

ConfigParser::ConfigParser() : _current_file(), _current_line(0)
{
}

ConfigParser::~ConfigParser()
{
}

Config ConfigParser::parse(const std::string *path)
{
    std::vector<Config> configs = parseMultiple(path);
    if (configs.empty())
    {
        throw std::runtime_error("No server configuration found in file");
    }
    return configs[0];
}

std::vector<Config> ConfigParser::parseMultiple(const std::string *path)
{
    if (!path)
    {
        throw std::runtime_error("Invalid configuration file path");
    }
    
    _current_file = std::string(path->substr());
    _current_line = 0;
    
    std::vector<std::string> lines = readConfigFile(path);
    std::vector<Config> configs;
    
    size_t i = 0;
    while (i < lines.size())
    {
        _current_line = i + 1;
        std::string line = trim(lines[i]);
        
        if (isEmptyOrComment(line))
        {
            i++;
            continue;
        }
        
        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty())
        {
            i++;
            continue;
        }
        
        if (tokens[0] == "server")
        {
            Config config;
            parseServerBlock(lines, i, config);
            configs.push_back(config);
        }
        else
        {
            throwError("Unexpected directive outside server block: " + tokens[0], i + 1);
        }
        
        i++;
    }
    
    return configs;
}

std::vector<std::string> ConfigParser::readConfigFile(const std::string *path)
{
    std::ifstream file(path->c_str());
    if (!file.is_open())
    {
        throw std::runtime_error(std::string("Failed to open configuration file: ") + path->substr());
    }
    
    std::vector<std::string> lines;
    std::string line;
    
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }
    
    file.close();
    return lines;
}

void ConfigParser::parseServerBlock(std::vector<std::string>& lines, size_t& index, Config& config)
{
    std::string line = trim(lines[index]);
    std::vector<std::string> tokens = tokenize(line);
    
    if (tokens.size() != 2 || tokens[0] != "server" || tokens[1] != "{")
    {
        throwError("Invalid server block syntax, expected 'server {'", index + 1);
    }
    
    index++;
    
    while (index < lines.size())
    {
        _current_line = index + 1;
        line = trim(lines[index]);
        
        if (isEmptyOrComment(line))
        {
            index++;
            continue;
        }
        
        tokens = tokenize(line);
        if (tokens.empty())
        {
            index++;
            continue;
        }
        
        if (tokens[0] == "}")
        {
            return;
        }
        
        if (tokens[0] == "location")
        {
            Location location;
            parseLocationBlock(lines, index, location);
            config.addLocation(location);
        }
        else
        {
            parseServerDirective(tokens[0], tokens, config);
        }
        
        index++;
    }
    
    throwError("Unclosed server block", index);
}

void ConfigParser::parseLocationBlock(std::vector<std::string>& lines, size_t& index, Location& location)
{
    std::string line = trim(lines[index]);
    std::vector<std::string> tokens = tokenize(line);
    
    if (tokens.size() < 3 || tokens[0] != "location" || tokens[tokens.size() - 1] != "{")
    {
        throwError("Invalid location block syntax, expected 'location <path> {'", index + 1);
    }
    
    location.setPath(tokens[1]);
    index++;
    
    while (index < lines.size())
    {
        _current_line = index + 1;
        line = trim(lines[index]);
        
        if (isEmptyOrComment(line))
        {
            index++;
            continue;
        }
        
        tokens = tokenize(line);
        if (tokens.empty())
        {
            index++;
            continue;
        }
        
        if (tokens[0] == "}")
        {
            return;
        }
        
        parseLocationDirective(tokens[0], tokens, location);
		LOG_DBG << "parseLocationDirective: " << location.isCgiEnabled();
        index++;
    }
    
    throwError("Unclosed location block", index);
}

void ConfigParser::parseServerDirective(const std::string& directive, const std::vector<std::string>& tokens, Config& config)
{
    if (directive == "listen")
    {
        if (tokens.size() < 2)
        {
            throwError("listen directive requires a value", _current_line);
        }
        std::string value = tokens[1];
        size_t colon = value.find(':');
        if (colon != std::string::npos)
        {
            config.setHost(value.substr(0, colon));
            config.setPort(stringToInt(value.substr(colon + 1)));
        }
        else
        {
            config.setPort(stringToInt(value));
        }
    }
    else if (directive == "server_name")
    {
        if (tokens.size() < 2)
        {
            throwError("server_name directive requires a value", _current_line);
        }
        if (tokens.size() > 1)
            config.setServerName(tokens[1]);
        for (size_t i = 2; i < tokens.size(); ++i)
            config.addServerName(tokens[i]);
    }
    else if (directive == "cgi-bin" || directive == "cgi_bin")
    {
        if (tokens.size() < 2)
            throwError("cgi-bin directive requires a value", _current_line);
        config.setCgiBinPath(tokens[1]);
    }
    else if (directive == "host")
    {
        if (tokens.size() < 2)
        {
            throwError("host directive requires a value", _current_line);
        }
        config.setHost(tokens[1]);
    }
    else if (directive == "root")
    {
        if (tokens.size() < 2)
        {
            throwError("root directive requires a value", _current_line);
        }
        config.setRoot(tokens[1]);
    }
    else if (directive == "client_max_body_size")
    {
        if (tokens.size() < 2)
        {
            throwError("client_max_body_size directive requires a value", _current_line);
        }
        std::string value = tokens[1];
        int multiplier = 1;
        
        if (!value.empty())
        {
            char last = value[value.length() - 1];
            if (last == 'M' || last == 'm')
            {
                multiplier = 1024 * 1024;
                value = value.substr(0, value.length() - 1);
            }
            else if (last == 'K' || last == 'k')
            {
                multiplier = 1024;
                value = value.substr(0, value.length() - 1);
            }
        }
        
        config.setClientMaxBodySize(stringToInt(value) * multiplier);
    }
    else if (directive == "index")
    {
        if (tokens.size() < 2)
        {
            throwError("index directive requires a value", _current_line);
        }
        config.setIndex(tokens[1]);
    }
    else if (directive == "error_page")
    {
        if (tokens.size() < 3)
        {
            throwError("error_page directive requires code and path", _current_line);
        }
        ErrorPage ep;
        ep.code = stringToInt(tokens[1]);
        ep.path = tokens[2];
        config.setErrorPage(ep);
    }
    else if (directive == "session")
    {
        if (tokens.size() < 2)
            throwError("session directive requires a value", _current_line);
        std::string v = tokens[1];
        if (v != "on" && v != "off")
            throwError("session directive must be 'on' or 'off'", _current_line);
        config.setSessionEnabled(v == "on");
    }
    else
    {
        throwError("Unknown server directive: " + directive, _current_line);
    }
}

void ConfigParser::parseLocationDirective(const std::string& directive, const std::vector<std::string>& tokens, Location& location)
{
    if (directive == "root")
    {
        if (tokens.size() < 2)
        {
            throwError("root directive requires a value", _current_line);
        }
        location.setRoot(tokens[1]);
    }
	else if (directive == "alias")
	{
		if (tokens.size() < 2)
		{
			throwError("alias directive requires a value", _current_line);
		}
		location.setAlias(tokens[1]);
	}
    else if (directive == "index")
    {
        if (tokens.size() < 2)
        {
            throwError("index directive requires a value", _current_line);
        }
        location.setIndex(tokens[1]);
    }
    else if (directive == "cgi")
    {
        if (tokens.size() < 2)
            throwError("cgi directive requires a value", _current_line);
        if (tokens[1] == "on" || tokens[1] == "off")
        {
            LOG_DBG << "parsing conf: cgi: " << tokens[1];
            location.setCgiEnabled(tokens[1] == "on");
        }
        else
        {
            for (size_t i = 1; i < tokens.size(); ++i)
                location.addCgiPath(tokens[i]);
            location.setCgiEnabled(true);
        }
    }
    else if (directive == "autoindex")
    {
        if (tokens.size() < 2)
        {
            throwError("autoindex directive requires a value", _current_line);
        }
        location.setAutoindex(stringToBool(tokens[1]));
    }
    else if (directive == "client_max_body_size")
    {
        if (tokens.size() < 2)
        {
            throwError("client_max_body_size directive requires a value", _current_line);
        }
        std::string value = tokens[1];
        int multiplier = 1;

        if (!value.empty())
        {
            char last = value[value.length() - 1];
            if (last == 'M' || last == 'm')
            {
                multiplier = 1024 * 1024;
                value = value.substr(0, value.length() - 1);
            }
            else if (last == 'K' || last == 'k')
            {
                multiplier = 1024;
                value = value.substr(0, value.length() - 1);
            }
        }
        location.setClientMaxBodySize(stringToInt(value) * multiplier);
    }
    else if (directive == "allow_methods" || directive == "allowed_methods" || directive == "limit_except")
    {
        for (size_t i = 1; i < tokens.size(); i++)
        {
            location.addAllowedMethod(tokens[i]);
        }
    }
    else if (directive == "return" || directive == "redirect")
    {
        if (tokens.size() < 2)
        {
            throwError("redirect directive requires a value", _current_line);
        }
        location.setRedirect(tokens[1]);
    }
    else if (directive == "cgi_path")
    {
        if (tokens.size() < 2)
        {
            throwError("cgi_path directive requires a value", _current_line);
        }
        for (size_t i = 1; i < tokens.size(); i++)
        {
            location.addCgiPath(tokens[i]);
        }
    }
    else if (directive == "cgi_ext")
    {
        if (tokens.size() < 2)
        {
            throwError("cgi_ext directive requires at least one value", _current_line);
        }
        for (size_t i = 1; i < tokens.size(); i++)
        {
            location.addCgiExt(tokens[i]);
        }
    }
    else if (directive == "upload_path" || directive == "upload")
    {
        if (tokens.size() < 2)
        {
            throwError("upload_path directive requires a value", _current_line);
        }
        location.setUploadPath(tokens[1]);
    }
    else if (directive == "auth_basic")
    {
        if (tokens.size() < 2)
            throwError("auth_basic directive requires a value", _current_line);
        location.setAuthBasic(tokens[1]);
    }
    else
    {
        throwError("Unknown location directive: " + directive, _current_line);
    }
}

std::string ConfigParser::trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
    {
        start++;
    }
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
    {
        end--;
    }
    
    return str.substr(start, end - start);
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    std::string current;
    
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == delimiter)
        {
            if (!current.empty())
            {
                result.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += str[i];
        }
    }
    
    if (!current.empty())
    {
        result.push_back(current);
    }
    
    return result;
}

std::vector<std::string> ConfigParser::tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    
    for (size_t i = 0; i < line.length(); i++)
    {
        char c = line[i];
        
        if (c == '"')
        {
            in_quotes = !in_quotes;
        }
        else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if (c == ';' && !in_quotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if ((c == '{' || c == '}') && !in_quotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else
        {
            current += c;
        }
    }
    
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    
    return tokens;
}

bool ConfigParser::isEmptyOrComment(const std::string& line)
{
    if (line.empty())
    {
        return true;
    }
    
    for (size_t i = 0; i < line.length(); i++)
    {
        if (!std::isspace(static_cast<unsigned char>(line[i])))
        {
            return line[i] == '#';
        }
    }
    
    return true;
}

int ConfigParser::stringToInt(const std::string& str)
{
    std::istringstream iss(str);
    int value;
    
    if (!(iss >> value))
    {
        throwError("Invalid integer value: " + str, _current_line);
    }
    
    return value;
}

bool ConfigParser::stringToBool(const std::string& str)
{
    std::string lower = str;
    for (size_t i = 0; i < lower.length(); i++)
    {
        lower[i] = std::tolower(static_cast<unsigned char>(lower[i]));
    }
    
    if (lower == "on" || lower == "true" || lower == "yes" || lower == "1")
    {
        return true;
    }
    else if (lower == "off" || lower == "false" || lower == "no" || lower == "0")
    {
        return false;
    }
    
    throwError("Invalid boolean value: " + str, _current_line);
    return false;
}

void ConfigParser::throwError(const std::string& message, size_t line_number)
{
    std::ostringstream oss;
    oss << "Config parse error in " << _current_file << ":" << line_number << " - " << message;
    throw std::runtime_error(oss.str());
}
