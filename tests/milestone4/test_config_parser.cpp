#include "config/ConfigParser.hpp"
#include "config/Config.hpp"
#include "config/Location.hpp"
#include <iostream>
#include <exception>

void printConfig(const Config& config)
{
    std::cout << "=== Server Configuration ===" << std::endl;
    std::cout << "Port: " << config.getPort() << std::endl;
    std::cout << "Server Name: " << config.getServerName() << std::endl;
    std::cout << "Host: " << config.getHost() << std::endl;
    std::cout << "Root: " << config.getRoot() << std::endl;
    std::cout << "Client Max Body Size: " << config.getClientMaxBodySize() << std::endl;
    std::cout << "Index: " << config.getIndex() << std::endl;
    
    ErrorPage ep = config.getErrorPage();
    if (ep.code != 0)
    {
        std::cout << "Error Page: " << ep.code << " -> " << ep.path << std::endl;
    }
    
    std::vector<Location> locations = config.getLocations();
    std::cout << "\nLocations (" << locations.size() << "):" << std::endl;
    
    for (size_t i = 0; i < locations.size(); i++)
    {
        std::cout << "\n  Location: " << locations[i].getPath() << std::endl;
        
        if (!locations[i].getRoot().empty())
        {
            std::cout << "    Root: " << locations[i].getRoot() << std::endl;
        }
        
        if (!locations[i].getIndex().empty())
        {
            std::cout << "    Index: " << locations[i].getIndex() << std::endl;
        }
        
        std::cout << "    Autoindex: " << (locations[i].getAutoindex() ? "on" : "off") << std::endl;
        
        std::vector<std::string> methods = locations[i].getAllowedMethods();
        if (!methods.empty())
        {
            std::cout << "    Allowed Methods: ";
            for (size_t j = 0; j < methods.size(); j++)
            {
                std::cout << methods[j];
                if (j < methods.size() - 1)
                {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        if (!locations[i].getRedirect().empty())
        {
            std::cout << "    Redirect: " << locations[i].getRedirect() << std::endl;
        }
        
        std::vector<std::string> cgi_paths = locations[i].getCgiPath();
        if (!cgi_paths.empty())
        {
            std::cout << "    CGI Paths: ";
            for (size_t j = 0; j < cgi_paths.size(); j++)
            {
                std::cout << cgi_paths[j];
                if (j < cgi_paths.size() - 1)
                {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        std::vector<std::string> cgi_exts = locations[i].getCgiExt();
        if (!cgi_exts.empty())
        {
            std::cout << "    CGI Extensions: ";
            for (size_t j = 0; j < cgi_exts.size(); j++)
            {
                std::cout << cgi_exts[j];
                if (j < cgi_exts.size() - 1)
                {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        if (!locations[i].getUploadPath().empty())
        {
            std::cout << "    Upload Path: " << locations[i].getUploadPath() << std::endl;
        }
    }
    
    std::cout << "\n============================\n" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    
    try
    {
        ConfigParser parser;
        
        std::cout << "Parsing configuration file: " << argv[1] << std::endl;
        std::cout << std::endl;
        std::string file = static_cast<const std::string>(argv[1]);

        std::vector<Config> configs = parser.parseMultiple(&file);
        
        std::cout << "Successfully parsed " << configs.size() << " server configuration(s)" << std::endl;
        std::cout << std::endl;
        
        for (size_t i = 0; i < configs.size(); i++)
        {
            std::cout << "Server Block " << (i + 1) << ":" << std::endl;
            printConfig(configs[i]);
        }
        
        std::cout << "Configuration parsing completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}