#include "utils/Log.hpp"
#include "utils/Logger.hpp"

void logutil::info(const char *msg)
{
	Logger::Info(msg);
}

void logutil::error(const char *msg)
{
	Logger::Error(msg);
}