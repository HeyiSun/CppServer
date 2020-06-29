#ifndef __CPPSERVER_LOG_H__
#define __CPPSERVER_LOG_H__

#include <string>
#include <cstdint>
#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <cstdarg>
#include <map>
#include "singleton.h"
#include "util.h"
#include "thread.h"

#define CPPSERVER_LOG_LEVEL(logger, level) \
    if (logger->getLevel() <= level)  \
        CppServer::LogEventWrap(CppServer::LogEvent::ptr(new CppServer::LogEvent(logger, \
                                level, __FILE__, __LINE__, 0, CppServer::GetThreadId(), \
                                CppServer::GetFiberId(), time(0)))).getSS()

#define CPPSERVER_LOG_DEBUG(logger) CPPSERVER_LOG_LEVEL(logger, CppServer::LogLevel::DEBUG)
#define CPPSERVER_LOG_INFO(logger) CPPSERVER_LOG_LEVEL(logger, CppServer::LogLevel::INFO)
#define CPPSERVER_LOG_WARN(logger) CPPSERVER_LOG_LEVEL(logger, CppServer::LogLevel::WARN)
#define CPPSERVER_LOG_ERROR(logger) CPPSERVER_LOG_LEVEL(logger, CppServer::LogLevel::ERROR)
#define CPPSERVER_LOG_FATAL(logger) CPPSERVER_LOG_LEVEL(logger, CppServer::LogLevel::FATAL)

#define CPPSERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if (logger->getLevel() <= level)  \
        CppServer::LogEventWrap(CppServer::LogEvent::ptr(new CppServer::LogEvent(logger, \
                                level, __FILE__, __LINE__, 0, CppServer::GetThreadId(), \
                  CppServer::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define CPPSERVER_LOG_fmt_DEBUG(logger, fmt, ...) CPPSERVER_LOG_FMT_LEVEL(logger, CppServer::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define CPPSERVER_LOG_fmt_INFO(logger, fmt, ...) CPPSERVER_LOG_FMT_LEVEL(logger, CppServer::LogLevel::INFO, fmt, __VA_ARGS__)
#define CPPSERVER_LOG_fmt_WARN(logger, fmt, ...) CPPSERVER_LOG_FMT_LEVEL(logger, CppServer::LogLevel::WARN, fmt, __VA_ARGS__)
#define CPPSERVER_LOG_fmt_ERROR(logger, fmt, ...) CPPSERVER_LOG_FMT_LEVEL(logger, CppServer::LogLevel::ERROR, fmt, __VA_ARGS__)
#define CPPSERVER_LOG_fmt_FATAL(logger, fmt, ...) CPPSERVER_LOG_FMT_LEVEL(logger, CppServer::LogLevel::FATAL, fmt, __VA_ARGS__)

#define CPPSERVER_LOG_ROOT() CppServer::LoggerMgr::GetInstance()->getRoot()
#define CPPSERVER_LOG_NAME(name) CppServer::LoggerMgr::GetInstance()->getLogger(name)

namespace CppServer {

class Logger;
class LoggerManager;

class LogLevel {
 public:
    enum Level {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};

class LogEvent {
 public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char* file, int32_t m_line, uint32_t elapse,
             uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    const char* getFile() const { return m_file; }
    uint32_t getLine() const { return m_line; }
    uint32_t getElapse() const { return m_elapse; }
    uint32_t getThreadId() const { return m_threadid; }
    uint32_t getFiberId() const { return m_fiberId; }
    uint64_t getTime() const { return m_time; }
    std::string getContent() const { return m_ss.str(); }
    std::shared_ptr<Logger> getLogger() const { return m_logger; }
    LogLevel::Level getLevel() const { return m_level; }

    std::stringstream& getSS() { return m_ss; }
    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
 private:
    const char* m_file = nullptr;   //   filename;
    int32_t m_line = 0;             //   line number
    uint32_t m_elapse = 0;          //   number of ms from program starting
    uint32_t m_threadid = 0;        //   thread id
    uint32_t m_fiberId = 0;         //   fiber thread id
    uint64_t m_time;                //   timestamp
    std::stringstream m_ss;

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
 public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event; }
    std::stringstream& getSS();
 private:
    LogEvent::ptr m_event;
};

class LogFormatter {
 public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    // %t %thread_id %m %n
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
 public:
    class FormatItem {
     public:
        typedef std::shared_ptr<FormatItem> ptr;
        FormatItem(const std::string& fmt = "") {}
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();

    bool isError() const { return m_error; }
    const std::string getPattern() const { return m_pattern; }
 private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_iterms;
    bool m_error = false;
};

// Log destination
class LogAppender {
 friend class Logger;
 public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef NullMutex MutexType;
    LogAppender() : m_level{LogLevel::DEBUG} {}
    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    virtual std::string toYamlString() = 0;

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    void setLevel(LogLevel::Level level) { m_level = level; }
    LogLevel::Level getLevel() const { return m_level; }
 protected:
    LogLevel::Level m_level;
    bool m_hasFormatter = false;
    MutexType m_mutex;
    LogFormatter::ptr m_formatter;
};

class Logger : public std::enable_shared_from_this<Logger> {
 friend class LoggerManager;
 public:
    typedef std::shared_ptr<Logger> ptr;
    typedef NullMutex MutexType;

    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level val) { m_level = val; }

    const std::string& getName() const { return m_name; }

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();

    std::string toYamlString();
 private:
    std::string m_name;                        // log name
    LogLevel::Level m_level;                   // log level
    MutexType m_mutex;
    std::list<LogAppender::ptr> m_appenders;   // Appender Collection
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};


// The Appender output to stdout
class StdoutLogAppender : public LogAppender {
 public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log (std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

 private:
};

// The Appender output to file
class FileLogAppender : public LogAppender {
 public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log (std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

    //reopen file, return true when successfully open
    bool reopen();
 private:
    std::string m_filename;
    std::ofstream m_filestream;
};

class LoggerManager {
 public:
    typedef NullMutex MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const { return m_root; }
    std::string toYamlString();
 private:
    MutexType m_mutex;
    std::map<std::string, Logger::ptr> m_loggers;
    Logger::ptr m_root;
};

typedef CppServer::Singleton<LoggerManager> LoggerMgr;

}  // CppServer


#endif  // __CPPSERVER_LOG_H__
