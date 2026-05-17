#include <gtest/gtest.h>
#include "service/filter_service.h"
#include "common/error_code.h"
#include <fstream>
#include <sstream>

using namespace chatroom;
using namespace chatroom::service;

class FilterServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::ofstream dictFile("test_dict.txt");
        dictFile << "bad,1\n";
        dictFile << "stupid,2\n";
        dictFile << "idiot,3\n";
        dictFile << "fool,1\n";
        dictFile << "damn,2\n";
        dictFile << "crap,1\n";
        dictFile.close();
        
        auto result = filterService.initialize("test_dict.txt");
        ASSERT_TRUE(result.isOk()) << result.message();
    }
    
    void TearDown() override {
        std::remove("test_dict.txt");
    }
    
    FilterService filterService;
};

TEST_F(FilterServiceTest, InitializeSuccess) {
    EXPECT_EQ(filterService.getDictionaryVersion(), 1);
    EXPECT_EQ(filterService.getDictionarySize(), 6);
}

TEST_F(FilterServiceTest, FilterNormalText) {
    auto result = filterService.filterText(1, "Hello World");
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_EQ(filtered.filteredText, "Hello World");
    EXPECT_TRUE(filtered.hitPositions.empty());
}

TEST_F(FilterServiceTest, FilterSingleSensitiveWord) {
    auto result = filterService.filterText(1, "This is bad text");
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_NE(filtered.filteredText.find("*"), std::string::npos);  // 检查有*号即可
    EXPECT_FALSE(filtered.hitPositions.empty());
    EXPECT_EQ(filtered.maxLevel, 1);
}

TEST_F(FilterServiceTest, FilterHighLevelWord) {
    auto result = filterService.filterText(1, "You are an idiot");
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_TRUE(filtered.wasBlocked);
    EXPECT_EQ(filtered.maxLevel, 3);
    for (char c : filtered.filteredText) {
        EXPECT_EQ(c, '*');
    }
}

TEST_F(FilterServiceTest, FilterMultipleWords) {
    auto result = filterService.filterText(1, "bad and stupid fool");
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_GE(filtered.hitPositions.size(), 3);
    EXPECT_GE(filtered.maxLevel, 2);
}

TEST_F(FilterServiceTest, EmptyText) {
    auto result = filterService.filterText(1, "");
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_TRUE(filtered.filteredText.empty());
}

TEST_F(FilterServiceTest, LongText) {
    std::string longText(10000, 'a');
    auto result = filterService.filterText(1, longText);
    ASSERT_TRUE(result.isOk());
    
    auto& filtered = result.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_EQ(filtered.filteredText.size(), 10000);
}

TEST_F(FilterServiceTest, ReloadDictionary) {
    {
        std::ofstream newDict("new_dict.txt");
        newDict << "newbad,1\n";
        newDict << "newword,2\n";
        newDict.close();
    }
    
    auto reloadResult = filterService.reloadDictionary("new_dict.txt");
    ASSERT_TRUE(reloadResult.isOk()) << reloadResult.message();
    
    EXPECT_EQ(filterService.getDictionaryVersion(), 2);
    EXPECT_EQ(filterService.getDictionarySize(), 2);
    
    auto filterResult = filterService.filterText(1, "This is newbad text");
    ASSERT_TRUE(filterResult.isOk());
    
    auto& filtered = filterResult.value();
    EXPECT_FALSE(filtered.wasBlocked);
    EXPECT_NE(filtered.filteredText.find("*"), std::string::npos);  // 检查有*号即可
    
    std::remove("new_dict.txt");
}

TEST_F(FilterServiceTest, UninitializedFilter) {
    FilterService uninitialized;
    
    auto result = uninitialized.filterText(1, "test");
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.code(), ErrorCode::InternalError);
}

TEST_F(FilterServiceTest, CaseSensitive) {
    auto resultLower = filterService.filterText(1, "this is bad");
    auto resultUpper = filterService.filterText(1, "this is BAD");
    
    ASSERT_TRUE(resultLower.isOk());
    ASSERT_TRUE(resultUpper.isOk());
    
    auto& filteredLower = resultLower.value();
    auto& filteredUpper = resultUpper.value();
    
    EXPECT_FALSE(filteredLower.hitPositions.empty());
    EXPECT_TRUE(filteredUpper.hitPositions.empty());
}

TEST(FilterServiceStandaloneTest, InvalidDictionaryPath) {
    FilterService service;
    
    auto result = service.initialize("/nonexistent/path/dict.txt");
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.code(), ErrorCode::NotFound);
}

TEST(FilterServiceStandaloneTest, EmptyDictionary) {
    std::ofstream emptyDict("empty_dict.txt");
    emptyDict.close();
    
    FilterService service;
    auto result = service.initialize("empty_dict.txt");
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.code(), ErrorCode::InvalidArgument);
    
    std::remove("empty_dict.txt");
}

TEST(FilterServiceStandaloneTest, ConcurrentAccess) {
    std::ofstream dictFile("concurrent_dict.txt");
    dictFile << "word1,1\n";
    dictFile << "word2,2\n";
    dictFile << "word3,1\n";
    dictFile.close();
    
    FilterService service;
    auto initResult = service.initialize("concurrent_dict.txt");
    ASSERT_TRUE(initResult.isOk());
    
    bool allPassed = true;
#pragma omp parallel for
    for (int i = 0; i < 100; i++) {
        auto result = service.filterText(i % 10, "test word1 word2 word3 text");
        if (!result.isOk() || result.value().wasBlocked) {
            allPassed = false;
        }
    }
    
    EXPECT_TRUE(allPassed);
    
    std::remove("concurrent_dict.txt");
}
