#include <iostream>
#include "../CppServer/log.h"
#include "../CppServer/util.h"

int main(int argc, char** argv) {
    CppServer::Logger::ptr logger(new CppServer::Logger);
    logger->addAppender(CppServer::LogAppender::ptr(new CppServer::StdoutLogAppender));
    CppServer::FileLogAppender::ptr file_appender(new CppServer::FileLogAppender("./log.txt"));
    logger->addAppender(file_appender);

    CppServer::LogFormatter::ptr fmt(new CppServer::LogFormatter("[%p]%d%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(CppServer::LogLevel::ERROR);

    // CppServer::LogEvent::ptr event(new CppServer::LogEvent(__FILE__, __LINE__, 0, CppServer::GetThreadId(), CppServer::GetFiberId(), time(0)));
    // event->getSS() << "Hello log";
    // logger->log(CppServer::LogLevel::DEBUG, event);

    CPPSERVER_LOG_INFO(logger) << "test macro";
    CPPSERVER_LOG_ERROR(logger) << "test macro error";
    CPPSERVER_LOG_fmt_FATAL(logger, "Hello %s", "World");
    return 0;
}
