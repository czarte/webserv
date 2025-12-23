#ifndef ROUTE_HPP
#define ROUTE_HPP

#include "Config.hpp"
#include "Location.hpp"
#include <string>

const Location *matchLocation(const Config &config, const std::string &target);

#endif // ROUTE_HPP
