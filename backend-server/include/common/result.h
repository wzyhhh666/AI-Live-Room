#pragma once

#include "error_code.h"
#include <string>
#include <utility>
#include <sstream>

namespace chatroom {

template<typename T>
class Result {
public:
    static Result ok(T value) {
        return Result(ErrorCode::Success, std::move(value), "");
    }

    static Result fail(ErrorCode code, const std::string& message = "") {
        return Result(code, T{}, message.empty() ? getErrorCodeMessage(code) : message);
    }

    bool isOk() const { return m_code == ErrorCode::Success; }
    explicit operator bool() const { return isOk(); }

    const T& value() const { return m_value; }
    T& value() { return m_value; }

    ErrorCode code() const { return m_code; }
    const std::string& message() const { return m_message; }

    std::string toString() const {
        std::ostringstream oss;
        oss << "Result{code=" << static_cast<int32_t>(m_code)
            << ", message='" << m_message << "'}";
        return oss.str();
    }

    Result orElse(T defaultValue) const {
        return isOk() ? *this : Result::ok(std::move(defaultValue));
    }

private:
    Result(ErrorCode code, T value, std::string message)
        : m_code(code), m_value(std::move(value)), m_message(std::move(message)) {}

    ErrorCode m_code;
    T m_value;
    std::string m_message;
};

template<>
class Result<void> {
public:
    static Result ok() {
        return Result(ErrorCode::Success);
    }

    static Result fail(ErrorCode code, const std::string& message = "") {
        return Result(code, message.empty() ? getErrorCodeMessage(code) : message);
    }

    bool isOk() const { return m_code == ErrorCode::Success; }
    explicit operator bool() const { return isOk(); }

    ErrorCode code() const { return m_code; }
    const std::string& message() const { return m_message; }

    std::string toString() const {
        std::ostringstream oss;
        oss << "Result{code=" << static_cast<int32_t>(m_code)
            << ", message='" << m_message << "'}";
        return oss.str();
    }

private:
    Result(ErrorCode code, const std::string& message = "")
        : m_code(code), m_message(message) {}

    ErrorCode m_code;
    std::string m_message;
};

using VoidResult = Result<void>;

} // namespace chatroom
