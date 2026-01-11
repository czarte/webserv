#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <sstream>
#include <iostream>

enum LogLevel
{
	LOG_DEBUG = 0,
	LOG_INFO = 1,
	LOG_WARN = 2,
	LOG_ERROR = 3,
	LOG_NONE = 4
};

class Logger
{
public:
	static Logger& getInstance();

	void setLevel(LogLevel level);
	LogLevel getLevel() const;

	void debug(const std::string& msg);
	void info(const std::string& msg);
	void warn(const std::string& msg);
	void error(const std::string& msg);

	// Template methods for convenient logging with stream-like syntax
	template<typename T>
	void debug(const T& msg)
	{
		std::ostringstream oss;
		oss << msg;
		debug(oss.str());
	}

	template<typename T>
	void info(const T& msg)
	{
		std::ostringstream oss;
		oss << msg;
		info(oss.str());
	}

	template<typename T>
	void warn(const T& msg)
	{
		std::ostringstream oss;
		oss << msg;
		warn(oss.str());
	}

	template<typename T>
	void error(const T& msg)
	{
		std::ostringstream oss;
		oss << msg;
		error(oss.str());
	}

	// Static convenience methods
	static void Debug(const std::string& msg);
	static void Info(const std::string& msg);
	static void Warn(const std::string& msg);
	static void Error(const std::string& msg);

private:
	Logger();
	~Logger();
	Logger(const Logger&);
	Logger& operator=(const Logger&);

	void log(LogLevel level, const std::string& msg);
	std::string getLevelString(LogLevel level) const;
	std::string getTimestamp() const;

	LogLevel _level;
};

// Stream-based logging helper
class LogStream
{
public:
	LogStream(LogLevel level);
	~LogStream();

	template<typename T>
	LogStream& operator<<(const T& value)
	{
		_stream << value;
		return *this;
	}

private:
	LogLevel _level;
	std::ostringstream _stream;
};

// Convenience macros for stream-based logging
#define LOG_DBG LogStream(LOG_DEBUG)
#define LOG_INF LogStream(LOG_INFO)
#define LOG_WRN LogStream(LOG_WARN)
#define LOG_ERR LogStream(LOG_ERROR)

#endif // LOGGER_HPP
