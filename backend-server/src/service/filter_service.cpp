#include "service/filter_service.h"
#include "common/logging.h"
#include <algorithm>
#include <cstring>

namespace chatroom {
namespace service {

FilterService::FilterService() = default;

FilterService::~FilterService() {
    AcNode* root = m_root.load(std::memory_order_acquire);
    if (root) {
        delete root;
        m_root.store(nullptr, std::memory_order_release);
    }
}

Result<void> FilterService::initialize(const std::string& dictPath) {
    LOG_INFO("FilterService: initializing with dictionary from {}", dictPath);
    
    auto wordsResult = loadDictionary(dictPath);
    if (!wordsResult.isOk()) {
        return VoidResult::fail(wordsResult.code(), wordsResult.message());
    }
    
    auto& words = wordsResult.value();
    if (words.empty()) {
        LOG_WARN("FilterService: dictionary is empty");
        return VoidResult::fail(ErrorCode::InvalidArgument, "Dictionary is empty");
    }
    
    std::lock_guard<std::mutex> lock(m_buildMutex);
    
    AcNode* newRoot = buildTrie(words);
    if (!newRoot) {
        return VoidResult::fail(ErrorCode::InternalError, "Failed to build Trie tree");
    }
    
    buildFailPointers(newRoot);
    
    AcNode* oldRoot = m_root.exchange(newRoot, std::memory_order_acq_rel);
    if (oldRoot) {
        delete oldRoot;
    }
    
    m_version.fetch_add(1, std::memory_order_acq_rel);
    m_dictSize.store(words.size(), std::memory_order_release);
    
    LOG_INFO("FilterService: initialized successfully, version={}, dict_size={}", 
             m_version.load(), words.size());
    
    return VoidResult::ok();
}

Result<FilterResult> FilterService::filterText(uint64_t roomId, const std::string& text) {
    AcNode* root = m_root.load(std::memory_order_acquire);
    if (!root) {
        return Result<FilterResult>::fail(ErrorCode::InternalError, "Filter not initialized");
    }
    
    FilterResult result = acMatch(root, text);
    
    if (result.wasBlocked) {
        LOG_WARN("FilterService: blocked text in room={}, max_level={}", roomId, result.maxLevel);
    } else if (!result.hitPositions.empty()) {
        LOG_DEBUG("FilterService: filtered text in room={}, hits={}", roomId, result.hitPositions.size());
    }
    
    return Result<FilterResult>::ok(result);
}

Result<void> FilterService::reloadDictionary(const std::string& newPath) {
    LOG_INFO("FilterService: reloading dictionary from {}", newPath);
    
    auto wordsResult = loadDictionary(newPath);
    if (!wordsResult.isOk()) {
        return VoidResult::fail(wordsResult.code(), wordsResult.message());
    }
    
    auto& words = wordsResult.value();
    if (words.empty()) {
        return VoidResult::fail(ErrorCode::InvalidArgument, "New dictionary is empty");
    }
    
    std::lock_guard<std::mutex> lock(m_buildMutex);
    
    AcNode* newRoot = buildTrie(words);
    if (!newRoot) {
        return VoidResult::fail(ErrorCode::InternalError, "Failed to rebuild Trie tree");
    }
    
    buildFailPointers(newRoot);
    
    AcNode* oldRoot = m_root.exchange(newRoot, std::memory_order_acq_rel);
    if (oldRoot) {
        delete oldRoot;
    }
    
    m_version.fetch_add(1, std::memory_order_acq_rel);
    m_dictSize.store(words.size(), std::memory_order_release);
    
    LOG_INFO("FilterService: dictionary reloaded successfully, version={}, dict_size={}", 
             m_version.load(), words.size());
    
    return VoidResult::ok();
}

uint64_t FilterService::getDictionaryVersion() const {
    return m_version.load(std::memory_order_acquire);
}

size_t FilterService::getDictionarySize() const {
    return m_dictSize.load(std::memory_order_acquire);
}

FilterService::AcNode* FilterService::buildTrie(const std::vector<std::pair<std::string, int>>& words) {
    auto root = new AcNode();
    int wordIdCounter = 0;
    
    for (const auto& [word, level] : words) {
        if (word.empty()) continue;
        
        AcNode* node = root;
        for (unsigned char ch : word) {
            if (!node->children[ch]) {
                node->children[ch] = new AcNode();
            }
            node = node->children[ch];
        }
        
        node->isEnd = true;
        node->wordId = wordIdCounter++;
        node->level = level;
    }
    
    LOG_DEBUG("FilterService: built Trie with {} words", wordIdCounter);
    return root;
}

void FilterService::buildFailPointers(AcNode* root) {
    std::queue<AcNode*> q;
    
    for (auto& child : root->children) {
        if (child) {
            child->fail = root;
            q.push(child);
        }
    }
    
    while (!q.empty()) {
        AcNode* current = q.front();
        q.pop();
        
        for (int i = 0; i < 128; i++) {
            AcNode* child = current->children[i];
            if (!child) continue;
            
            AcNode* fail = current->fail;
            while (fail && !fail->children[i]) {
                fail = fail->fail;
            }
            
            child->fail = fail ? fail->children[i] : root;
            q.push(child);
        }
    }
    
    LOG_DEBUG("FilterService: built fail pointers");
}

FilterResult FilterService::acMatch(AcNode* root, const std::string& text) {
    FilterResult result;
    result.filteredText = text;
    
    if (text.empty()) {
        return result;
    }
    
    AcNode* current = root;
    size_t len = text.size();
    
    for (size_t i = 0; i < len; ) {
        unsigned char ch = static_cast<unsigned char>(text[i]);
        
        if (current->children[ch]) {
            current = current->children[ch];
            
            if (current->isEnd) {
                AcNode* temp = current;
                int maxLevelFound = 0;
                
                while (temp && temp != root) {
                    if (temp->isEnd && temp->level > maxLevelFound) {
                        maxLevelFound = temp->level;
                    }
                    temp = temp->fail;
                }
                
                if (maxLevelFound >= 3) {
                    result.wasBlocked = true;
                    result.maxLevel = maxLevelFound;
                    
                    for (size_t j = 0; j < len; j++) {
                        result.filteredText[j] = '*';
                    }
                    
                    result.hitPositions.emplace_back(0, static_cast<int>(len));
                    return result;
                } else if (maxLevelFound > 0) {
                    size_t wordLen = getWordLength(current, root);
                    size_t start = (i + 1 > wordLen) ? (i + 1 - wordLen) : 0;
                    
                    for (size_t j = start; j <= i && j < len; j++) {
                        result.filteredText[j] = '*';
                    }
                    
                    result.hitPositions.emplace_back(static_cast<int>(start), 
                                                      static_cast<int>(i - start + 1));
                    
                    if (maxLevelFound > result.maxLevel) {
                        result.maxLevel = maxLevelFound;
                    }
                }
            }
            
            ++i;
        } else {
            if (current == root) {
                ++i;
            } else {
                current = current->fail ? current->fail : root;
            }
        }
    }
    
    return result;
}

size_t FilterService::getWordLength(AcNode* node, AcNode* root) {
    size_t length = 0;
    while (node && node != root) {
        length++;
        
        bool foundParent = false;
        for (int i = 0; i < 128; i++) {
            if (node->fail && node->fail->children[i] == node) {
                node = node->fail;
                foundParent = true;
                break;
            }
        }
        
        if (!foundParent) break;
    }
    return length;
}

Result<std::vector<std::pair<std::string, int>>> FilterService::loadDictionary(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<std::vector<std::pair<std::string, int>>>::fail(
            ErrorCode::NotFound, "Cannot open dictionary file: " + path);
    }
    
    std::vector<std::pair<std::string, int>> words;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string word;
        int level = 1;
        
        if (std::getline(iss, word, ',')) {
            word.erase(0, word.find_first_not_of(" \t"));
            word.erase(word.find_last_not_of(" \t") + 1);
            
            if (!(iss >> level)) {
                level = 1;
            }
            
            if (level < 1) level = 1;
            if (level > 3) level = 3;
            
            if (!word.empty()) {
                words.emplace_back(word, level);
            }
        }
    }
    
    LOG_INFO("FilterService: loaded {} words from dictionary", words.size());
    return Result<std::vector<std::pair<std::string, int>>>::ok(words);
}

} // namespace service
} // namespace chatroom
