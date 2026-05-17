#include "util/utils.h"

namespace chatroom {

namespace utils {

std::string generateSecureToken() {
    static std::mt19937_64 generator(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(0, 255);
    
    static const char hex_chars[] = "0123456789abcdef";
    std::string token;
    token.reserve(32);
    
    for (int i = 0; i < 32; i++) {
        unsigned char byte = distribution(generator);
        token += hex_chars[(byte >> 4) & 0x0F];
        token += hex_chars[byte & 0x0F];
    }
    
    return token;
}

} // namespace utils

} // namespace chatroom
