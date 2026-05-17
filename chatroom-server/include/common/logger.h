#pragma once

#include <spdlog/spdlog.h>
#include <memory>

namespace chatroom {

class Logger {
public:
    static void init(const std::string& level = "INFO",
                     const std::string& filePath = "logs/chatroom.log",
                     size_t maxFileSize = 104857600,
                     size_t maxFiles = 10);

    static std::shared_ptr<spdlog::logger>& getLogger();

private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

#define LOG_TRACE(...)   SPDLOG_LOGGER_TRACE(chatroom::Logger::getLogger(), __VA_ARGS__)
#define LOG_DEBUG(...)   SPDLOG_LOGGER_DEBUG(chatroom::Logger::getLogger(), __VA_ARGS__)
#define LOG_INFO(...)    SPDLOG_LOGGER_INFO(chatroom::Logger::getLogger(), __VA_ARGS__)
#define LOG_WARN(...)    SPDLOG_LOGGER_WARN(chatroom::Logger::getLogger(), __VA_ARGS__)
#define LOG_ERROR(...)   SPDLOG_LOGGER_ERROR(chatroom::Logger::getLogger(), __VA_ARGS__)
#define LOG_FATAL(...)   SPDLOG_LOGGER_CRITICAL(chatroom::Logger::getLogger(), __VA_ARGS__)

} // namespace chatroom
