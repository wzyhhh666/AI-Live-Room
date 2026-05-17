#include "common/logging.h"
#include <iostream>

namespace chatroom {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& logFilePath,
                         spdlog::level::level_enum consoleLevel,
                         spdlog::level::level_enum fileLevel,
                         size_t maxFileSize,
                         size_t maxFiles) {
    try {
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l][%n][tid=%t][%v]");

        if (!m_consoleLogger) {
            m_consoleLogger = spdlog::stdout_color_mt("console");
            m_consoleLogger->set_level(consoleLevel);
            m_consoleLogger->flush_on(spdlog::level::err);
        } else {
            m_consoleLogger->set_level(consoleLevel);
        }

        if (!m_fileLogger) {
            m_fileLogger = spdlog::rotating_logger_mt("file", logFilePath, maxFileSize, maxFiles);
            m_fileLogger->set_level(fileLevel);
            m_fileLogger->flush_on(spdlog::level::err);
        } else {
            m_fileLogger->set_level(fileLevel);
        }

        LOG_INFO("Logger initialized: console_level={}, file_level={}, file={}", 
                 static_cast<int>(consoleLevel), static_cast<int>(fileLevel), logFilePath);

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization warning: " << ex.what() << std::endl;
    }
}

void Logger::flush() {
    if (m_consoleLogger) m_consoleLogger->flush();
    if (m_fileLogger) m_fileLogger->flush();
}

} // namespace chatroom
