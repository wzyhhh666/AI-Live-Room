#pragma once

#include <cstdint>

namespace chatroom {

enum class ErrorCode : int32_t {
    Success = 0,
    InvalidArgument = 1001,
    ProtocolError = 1002,
    NetworkError = 1003,
    DatabaseError = 1004,
    NotFound = 1005,
    PermissionDenied = 1006,
    RateLimited = 1007,
    InternalError = 1008,
    Timeout = 1009,
    AlreadyExists = 1010,
    ConfigError = 1011
};

inline const char* getErrorCodeMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:           return "Success";
        case ErrorCode::InvalidArgument:   return "Invalid argument";
        case ErrorCode::ProtocolError:     return "Protocol error";
        case ErrorCode::NetworkError:      return "Network error";
        case ErrorCode::DatabaseError:     return "Database error";
        case ErrorCode::NotFound:          return "Not found";
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::RateLimited:       return "Rate limited";
        case ErrorCode::InternalError:     return "Internal error";
        case ErrorCode::Timeout:           return "Timeout";
        case ErrorCode::AlreadyExists:     return "Already exists";
        case ErrorCode::ConfigError:       return "Configuration error";
        default:                          return "Unknown error";
    }
}

} // namespace chatroom
