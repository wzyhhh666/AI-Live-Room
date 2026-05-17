#pragma once

#include <atomic>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <fstream>
#include <sstream>
#include "common/result.h"

namespace chatroom {
namespace service {

struct FilterResult {
    std::string filteredText;
    bool wasBlocked = false;
    std::vector<std::pair<int, int>> hitPositions;
    int maxLevel = 0;
};

class FilterService {
public:
    FilterService();
    ~FilterService();
    
    Result<void> initialize(const std::string& dictPath);
    
    Result<FilterResult> filterText(uint64_t roomId, const std::string& text);
    
    Result<void> reloadDictionary(const std::string& newPath);
    
    uint64_t getDictionaryVersion() const;
    
    size_t getDictionarySize() const;

private:
    struct AcNode {
        std::array<AcNode*, 128> children{};
        AcNode* fail = nullptr;
        bool isEnd = false;
        int wordId = -1;
        int level = 0;
        
        AcNode() = default;
        ~AcNode() {
            for (auto child : children) {
                delete child;
            }
        }
    };
    
    AcNode* buildTrie(const std::vector<std::pair<std::string, int>>& words);
    
    void buildFailPointers(AcNode* root);
    
    FilterResult acMatch(AcNode* root, const std::string& text);
    
    size_t getWordLength(AcNode* node, AcNode* root);
    
    Result<std::vector<std::pair<std::string, int>>> loadDictionary(const std::string& path);
    
    std::atomic<AcNode*> m_root{nullptr};
    std::atomic<uint64_t> m_version{0};
    std::atomic<size_t> m_dictSize{0};
    std::mutex m_buildMutex;
};

} // namespace service
} // namespace chatroom
