#include <gtest/gtest.h>
#include "util/config_manager.h"
#include "common/logging.h"

using namespace chatroom;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().initialize("/dev/null", spdlog::level::off, spdlog::level::off);
    }
};

TEST_F(ConfigTest, DefaultValues) {
    ConfigManager& mgr = ConfigManager::getInstance();
    const AppConfig& config = mgr.getConfig();
    
    EXPECT_EQ(config.server.host, "0.0.0.0");
    EXPECT_EQ(config.server.port, 8900);
    EXPECT_EQ(config.mysql.host, "127.0.0.1");
    EXPECT_EQ(config.redis.port, 6379);
    EXPECT_EQ(config.heartbeat.clientIntervalSec, 10);
}

TEST_F(ConfigTest, LoadFromFile) {
    ConfigManager& mgr = ConfigManager::getInstance();
    
    Result<void> result = mgr.loadFromFile("../config/app.json");
    if (!result.isOk()) {
        std::cout << "Load config failed: " << result.message() 
                  << ", code=" << static_cast<int>(result.code()) << std::endl;
    }
    ASSERT_TRUE(result.isOk());
    
    const AppConfig& config = mgr.getConfig();
    EXPECT_EQ(config.server.port, 8900);
    EXPECT_EQ(config.mysql.database, "chatroom_db");
    EXPECT_EQ(config.rateLimit.userMaxPerSec, 1);
}

TEST_F(ConfigTest, LoadNonExistentFile) {
    ConfigManager& mgr = ConfigManager::getInstance();
    
    Result<void> result = mgr.loadFromFile("nonexistent.json");
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.code(), ErrorCode::ConfigError);
}
