#include <gtest/gtest.h>
#include "util/timer_wheel.h"
#include <atomic>

using namespace chatroom;

class TimerWheelTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TimerWheelTest, AddAndTrigger) {
    util::TimerWheel wheel(1024, 100);
    
    std::atomic<bool> triggered{false};
    uint64_t timerId = wheel.addTimer(200, [&triggered]() { triggered.store(true); });
    
    ASSERT_NE(timerId, 0);
    
    wheel.tick();
    EXPECT_FALSE(triggered.load());
    
    wheel.tick();
    EXPECT_FALSE(triggered.load());
    
    wheel.tick();
    EXPECT_TRUE(triggered.load());
}

TEST_F(TimerWheelTest, CancelTimer) {
    util::TimerWheel wheel(1024, 100);
    
    std::atomic<int> count{0};
    uint64_t timerId = wheel.addTimer(300, [&count]() { count++; });
    
    bool cancelled = wheel.cancelTimer(timerId);
    EXPECT_TRUE(cancelled);
    
    for (int i = 0; i < 5; ++i) {
        wheel.tick();
    }
    
    EXPECT_EQ(count.load(), 0);
}

TEST_F(TimerWheelTest, MultipleTimers) {
    util::TimerWheel wheel(1024, 50);
    
    std::atomic<int> count{0};
    
    wheel.addTimer(100, [&count]() { count++; });
    wheel.addTimer(150, [&count]() { count++; });
    wheel.addTimer(200, [&count]() { count++; });
    
    for (int i = 0; i < 5; ++i) {
        wheel.tick();
    }
    
    EXPECT_EQ(count.load(), 3);
}

TEST_F(TimerWheelTest, ZeroTimeout) {
    util::TimerWheel wheel(1024, 100);
    
    uint64_t timerId = wheel.addTimer(0, [](){});
    EXPECT_EQ(timerId, 0);
}

TEST_F(TimerWheelTest, PendingCount) {
    util::TimerWheel wheel(1024, 100);
    
    EXPECT_EQ(wheel.getPendingTimerCount(), size_t{0});
    
    wheel.addTimer(500, [](){});
    wheel.addTimer(600, [](){});
    
    EXPECT_EQ(wheel.getPendingTimerCount(), size_t{2});
}
