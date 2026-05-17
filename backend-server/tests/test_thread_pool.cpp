#include <gtest/gtest.h>
#include <future>
#include "util/thread_pool.h"

using namespace chatroom;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ThreadPoolTest, BasicSubmit) {
    util::ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    auto result = pool.submit([&counter]() { counter++; });
    
    ASSERT_TRUE(result.isOk());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(counter.load(), 1);
}

TEST_F(ThreadPoolTest, MultipleTasks) {
    util::ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter]() { counter++; });
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, TaskIdUnique) {
    util::ThreadPool pool(2);
    
    std::vector<uint64_t> taskIds;
    for (int i = 0; i < 10; ++i) {
        auto result = pool.submit([](){});
        if (result.isOk()) {
            taskIds.push_back(result.value());
        }
    }
    
    std::sort(taskIds.begin(), taskIds.end());
    auto uniqueEnd = std::unique(taskIds.begin(), taskIds.end());
    EXPECT_EQ(std::distance(taskIds.begin(), uniqueEnd), static_cast<long>(taskIds.size()));
}

TEST_F(ThreadPoolTest, Shutdown) {
    util::ThreadPool pool(2);
    
    EXPECT_TRUE(pool.isRunning());
    
    pool.shutdown();
    
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(ThreadPoolTest, SubmitAfterShutdown) {
    util::ThreadPool pool(2);
    pool.shutdown();
    
    auto result = pool.submit([](){});
    
    EXPECT_FALSE(result.isOk());
}
