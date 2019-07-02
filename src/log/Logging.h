#ifndef _SPDLOG_LOGGER_
#define _SPDLOG_LOGGER_

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else 
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <iostream>
#include <mutex>
#include "MQClient.h"

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif

// define log SUFFIX: [filename] [function:line] macro
#ifndef SUFFIX
#define SUFFIX(msg)  std::string(msg).append(" [")\
        .append(__FILENAME__).append("] [").append(__func__)\
        .append(":").append(std::to_string(__LINE__))\
        .append("]").c_str()
#endif

// must define before spdlog.h
#ifndef SPDLOG_TRACE_ON
#define SPDLOG_TRACE_ON
#endif

#ifndef SPDLOG_DEBUG_ON
#define SPDLOG_DEBUG_ON
#endif

#include "spdlog.h"
#include "async.h"
#include "sinks/stdout_color_sinks.h"
#include "sinks/basic_file_sink.h"
#include "sinks/rotating_file_sink.h"

namespace rocketmq {

class logAdapter {
public:
    ~logAdapter();
    static logAdapter* getLogInstance();
    void setLogLevel(elogLevel logLevel);
    elogLevel getLogLevel();
    void setLogFileNumAndSize(int logNum, int sizeOfPerFile);

	//auto getLogger() { return m_logger; } // c++14 or warning
	std::shared_ptr<spdlog::async_logger>& getLogger() { return m_logger; }

private:
	logAdapter(const logAdapter&) = delete;
	logAdapter& operator=(const logAdapter&) = delete;

private:
    logAdapter();
    void setLogLevelInner(elogLevel logLevel);
    elogLevel m_logLevel;
    std::string m_logFile;
	std::shared_ptr<spdlog::async_logger> m_logger;
    std::vector<spdlog::sink_ptr> m_logSinks;
    static logAdapter* alogInstance;
    static std::mutex m_imtx;
};

#define ALOG_ADAPTER logAdapter::getLogInstance()

#define AGENT_LOGGER AGENT_LOGGER()->getLogger()

#define LOG_TRACE(msg, ...) logAdapter::getLogInstance()->getLogger()->trace(SUFFIX(msg), ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) logAdapter::getLogInstance()->getLogger()->debug(SUFFIX(msg), ##__VA_ARGS__)
#define LOG_INFO(msg, ...) logAdapter::getLogInstance()->getLogger()->info(SUFFIX(msg), ##__VA_ARGS__)
#define LOG_WARN(msg, ...) logAdapter::getLogInstance()->getLogger()->warn(SUFFIX(msg), ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) logAdapter::getLogInstance()->getLogger()->error(SUFFIX(msg), ##__VA_ARGS__)
#define LOG_CRITICAL(msg, ...) logAdapter::getLogInstance()->getLogger()->critical(SUFFIX(msg), ##__VA_ARGS__)

#define criticalif(b, ...)                        \
    do {                                       \
        if ((b)) {                             \
           logAdapter::getLogInstance()->getLogger()->critical(__VA_ARGS__); \
        }                                      \
    } while (0)

#ifdef WIN32  
#define errcode WSAGetLastError()
#endif
} // namespace rocketmq
#endif
