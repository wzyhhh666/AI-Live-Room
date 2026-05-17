#pragma once

#include <string>
#include <utility>

namespace chatroom {

enum class ErrorCode {
    Success = 0,
    InvalidArgument,
    ProtocolError,
    NetworkError,
    DatabaseError,
    NotFound,
    PermissionDenied,
    RateLimited,
    InternalError
};

template <typename T>
class Result {
public:
    static Result ok(T value) {
        return Result(ErrorCode::Success, std::move(value), "");
    }

    static Result fail(ErrorCode code, std::string message) {
        return Result(code, T{}, std::move(message));
    }

    bool isOk() const { return m_code == ErrorCode::Success; }
    const T& value() const& { return m_value; }
    T value() && { return std::move(m_value); }
    ErrorCode code() const { return m_code; }
    const std::string& message() const { return m_message; }

private:
    Result(ErrorCode code, T value, std::string message)
        : m_code(code), m_value(std::move(value)), m_message(std::move(message)) {}

    ErrorCode m_code;
    T m_value;
    std::string m_message;
};

} // namespace chatroom
