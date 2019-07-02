#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {
    logAdapter* logAdapter::alogInstance = nullptr;
    std::mutex logAdapter::m_imtx;

    logAdapter::~logAdapter() {
		spdlog::drop_all();
    }

    logAdapter* logAdapter::getLogInstance() {
        if (alogInstance == nullptr) {
            std::lock_guard<std::mutex> mtx(m_imtx);
            if (alogInstance == NULL) {
                alogInstance = new logAdapter();
            }
        }
        return alogInstance;
    }

    logAdapter::logAdapter() : m_logLevel(eLOG_LEVEL_INFO) {
        string homeDir(UtilAll::getHomeDirectory());
        homeDir.append("/logs/rocketmq-cpp/");
        UtilAll::createDirectory(homeDir);
        if (!UtilAll::existDirectory(homeDir)) {
            std::cerr << "create log dir error, will exit" << std::endl;
            exit(1);
        }
        m_logFile += homeDir;
        std::string fileName = UtilAll::to_string(getpid()) + "_" + "rocketmq-cpp.log";
        m_logFile += fileName;

#ifdef _DEBUG
        auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console->set_level(spdlog::level::debug);
        console->set_pattern("[%m-%d %H:%M:%S.%e][%^%L%$][thread:%t]  %v");
        m_logSinks.push_back(console);
#endif
        auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_logFile, 1024 * 1024 * 1024, 5);
        rotating->set_level(spdlog::level::debug);
        rotating->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$][thread:%t]  %v");
        m_logSinks.push_back(rotating);

        spdlog::init_thread_pool(8192, 1);
        m_logger = std::make_shared<spdlog::async_logger>("both", begin(m_logSinks), end(m_logSinks), spdlog::thread_pool());
        //register it if you need to access it globally
        spdlog::register_logger(m_logger);

        // set log level
        /*
#ifdef _DEBUG
        m_logger->set_level(spdlog::level::trace);
#else
        m_logger->set_level(spdlog::level::info);
#endif
*/
        setLogLevelInner(m_logLevel);
        //when an error occurred, flush disk immediately
        m_logger->flush_on(spdlog::level::err);

        spdlog::flush_every(std::chrono::seconds(3));

    }

    void logAdapter::setLogLevel(elogLevel logLevel) {
        m_logLevel = logLevel;
        setLogLevelInner(m_logLevel);
    }

    elogLevel logAdapter::getLogLevel() {
        return m_logLevel;
    }

    void logAdapter::setLogFileNumAndSize(int logNum, int sizeOfPerFile) {
    }

    void logAdapter::setLogLevelInner(elogLevel logLevel) {
        /*
           eLOG_LEVEL_FATAL = 1,
           eLOG_LEVEL_ERROR = 2,
           eLOG_LEVEL_WARN = 3,
           eLOG_LEVEL_INFO = 4,
           eLOG_LEVEL_DEBUG = 5,
           eLOG_LEVEL_TRACE = 6,
           */
        switch (logLevel) {
            case eLOG_LEVEL_FATAL:
                m_logger->set_level(spdlog::level::critical);
                break;
            case eLOG_LEVEL_ERROR:
                m_logger->set_level(spdlog::level::err);
                break;
            case eLOG_LEVEL_WARN:
                m_logger->set_level(spdlog::level::warn);
                break;
            case eLOG_LEVEL_INFO:
                m_logger->set_level(spdlog::level::info);
                break;
            case eLOG_LEVEL_DEBUG:
                m_logger->set_level(spdlog::level::debug);
                break;
            case eLOG_LEVEL_TRACE:
                m_logger->set_level(spdlog::level::trace);
                break;
            default:
                m_logger->set_level(spdlog::level::info);
                break;
        }
    }

} // namespace rocketmq

