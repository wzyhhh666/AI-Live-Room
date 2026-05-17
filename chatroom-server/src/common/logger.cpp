#define SPDLOG_HEADER_ONLY
#include "common/logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace chatroom {

std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;

void Logger::init(const std::string& level,
                  const std::string& filePath,
                  size_t maxFileSize,
                  size_t maxFiles) {
    std::vector<spdlog::sink_ptr> sinks;

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%^%l%$][%n] %v");
    sinks.push_back(consoleSink);

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filePath, maxFileSize, maxFiles);
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l][%n][tid=%t] %v");
    sinks.push_back(fileSink);

    s_logger = std::make_shared<spdlog::logger>("chatroom", begin(sinks), end(sinks));
    s_logger->set_level(spdlog::level::from_str(level));
    s_logger->flush_on(spdlog::level::err);
    spdlog::register_logger(s_logger);
}

std::shared_ptr<spdlog::logger>& Logger::getLogger() {
    if (!s_logger) {
        init();
    }
    return s_logger;
}

} // namespace chatroom
