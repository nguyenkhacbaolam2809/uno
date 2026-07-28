#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <cstdarg>

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger & instance();

    void setLevel(LogLevel minLevel);
    void enableFileOutput(const std::string & path);
    void disableFileOutput();

    void log(LogLevel level, const char * file, int line, const char * fmt, ...);
    void logv(LogLevel level, const char * file, int line, const char * fmt, va_list args);

    static const char * levelName(LogLevel level);

private:
    Logger();
    ~Logger();
    Logger(const Logger &) = delete;
    Logger & operator=(const Logger &) = delete;

    LogLevel m_minLevel;
    std::ofstream m_file;
    std::mutex m_mutex;
    bool m_fileEnabled;
};

#define LOG_TRACE(fmt, ...)   Logger::instance().log(LogLevel::TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)   Logger::instance().log(LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    Logger::instance().log(LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)    Logger::instance().log(LogLevel::WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   Logger::instance().log(LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)   Logger::instance().log(LogLevel::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif
