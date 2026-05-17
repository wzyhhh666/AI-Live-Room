#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace chatroom {

class Logger {
public:
    static Logger& getInstance();

    void initialize(const std::string& logFilePath = "logs/chatroom.log",
                    spdlog::level::level_enum consoleLevel = spdlog::level::info,
                    spdlog::level::level_enum fileLevel = spdlog::level::debug,
                    size_t maxFileSize = 104857600,
                    size_t maxFiles = 10);

    std::shared_ptr<spdlog::logger> getConsoleLogger() { return m_consoleLogger; }
    std::shared_ptr<spdlog::logger> getFileLogger() { return m_fileLogger; }

    template<typename... Args>
    void trace(const char* fmt, Args&&... args) {
        m_consoleLogger->trace(fmt, std::forward<Args>(args)...);
        m_fileLogger->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const char* fmt, Args&&... args) {
        m_consoleLogger->debug(fmt, std::forward<Args>(args)...);
        m_fileLogger->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char* fmt, Args&&... args) {
        m_consoleLogger->info(fmt, std::forward<Args>(args)...);
        m_fileLogger->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char* fmt, Args&&... args) {
        m_consoleLogger->warn(fmt, std::forward<Args>(args)...);
        m_fileLogger->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char* fmt, Args&&... args) {
        m_consoleLogger->error(fmt, std::forward<Args>(args)...);
        m_fileLogger->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(const char* fmt, Args&&... args) {
        m_consoleLogger->critical(fmt, std::forward<Args>(args)...);
        m_fileLogger->critical(fmt, std::forward<Args>(args)...);
    }

    void flush();

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> m_consoleLogger;
    std::shared_ptr<spdlog::logger> m_fileLogger;
};

#define LOG_TRACE(fmt, ...) chatroom::Logger::getInstance().trace(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) chatroom::Logger::getInstance().debug(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  chatroom::Logger::getInstance().info(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  chatroom::Logger::getInstance().warn(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) chatroom::Logger::getInstance().error(fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) chatroom::Logger::getInstance().critical(fmt, ##__VA_ARGS__)

} // namespace chatroom
