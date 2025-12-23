#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "./Config.hpp"
#include "./Location.hpp"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

class ConfigParser
{
public:
    ConfigParser();
    ~ConfigParser();

    Config parse(const std::string *path);
    std::vector<Config> parseMultiple(const std::string *path);

private:
    // Copy prevention (C++98 style)
    ConfigParser(const ConfigParser&);
    ConfigParser& operator=(const ConfigParser&);

    // Parsing utilities
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<std::string> tokenize(const std::string& line);
    bool isEmptyOrComment(const std::string& line);

    // Block parsing
    void parseServerBlock(std::vector<std::string>& lines, size_t& index, Config& config);
    void parseLocationBlock(std::vector<std::string>& lines, size_t& index, Location& location);

    // Directive parsing
    void parseServerDirective(const std::string& directive, const std::vector<std::string>& tokens, Config& config);
    void parseLocationDirective(const std::string& directive, const std::vector<std::string>& tokens, Location& location);

    // Conversion utilities
    int stringToInt(const std::string& str);
    bool stringToBool(const std::string& str);

    // File reading
    std::vector<std::string> readConfigFile(const std::string *path);

    // Error handling
    void throwError(const std::string& message, size_t line_number);

    // State
    std::string _current_file;
    size_t _current_line;
};

#endif
