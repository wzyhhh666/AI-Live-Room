#include <gtest/gtest.h>
#include "common/result.h"
#include "common/error_code.h"

using namespace chatroom;

TEST(ResultTest, OkResult) {
    Result<int> result = Result<int>::ok(42);
    
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(result);
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(result.code(), ErrorCode::Success);
    EXPECT_EQ(result.message(), "");
}

TEST(ResultTest, FailResult) {
    Result<std::string> result = Result<std::string>::fail(
        ErrorCode::InvalidArgument, "test error");
    
    EXPECT_FALSE(result.isOk());
    EXPECT_FALSE(result);
    EXPECT_EQ(result.code(), ErrorCode::InvalidArgument);
    EXPECT_EQ(result.message(), "test error");
}

TEST(ResultTest, VoidResult) {
    VoidResult okResult = VoidResult::ok();
    EXPECT_TRUE(okResult.isOk());
    
    VoidResult failResult = VoidResult::fail(ErrorCode::NetworkError);
    EXPECT_FALSE(failResult.isOk());
    EXPECT_EQ(failResult.code(), ErrorCode::NetworkError);
}

TEST(ResultTest, OrElse) {
    Result<int> okResult = Result<int>::ok(100);
    Result<int> failResult = Result<int>::fail(ErrorCode::NotFound);
    
    auto value1 = okResult.orElse(0);
    EXPECT_EQ(value1.value(), 100);
    
    auto value2 = failResult.orElse(999);
    EXPECT_EQ(value2.value(), 999);
}

TEST(ResultTest, ToString) {
    Result<double> ok = Result<double>::ok(3.14);
    EXPECT_FALSE(ok.toString().empty());
    
    Result<int> fail = Result<int>::fail(ErrorCode::Timeout, "custom msg");
    std::string str = fail.toString();
    EXPECT_NE(str.find("custom msg"), std::string::npos);
    EXPECT_NE(str.find(std::to_string(static_cast<int>(ErrorCode::Timeout))), std::string::npos);
}
