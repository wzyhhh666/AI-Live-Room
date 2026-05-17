#pragma once

#include <string>
#include <variant>

namespace chatroom {

enum ErrorCode {
    Ok = 0,
    InvalidArgument = -1,
    NotFound = -2,
    PermissionDenied = -3,
    InternalError = -4,
    ProtocolError = -5,
    Timeout = -6
};

template<typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.m_value = std::move(value);
        return r;
    }

    static Result fail(int code, const std::string& message) {
        Result r;
        r.m_errorCode = code;
        r.m_errorMessage = message;
        r.m_value = std::monostate{};
        return r;
    }

    bool isOk() const { 
        return std::holds_alternative<T>(m_value); 
    }

    T& value() { 
        return std::get<T>(m_value); 
    }
    
    const T& value() const { 
        return std::get<T>(m_value); 
    }

    int code() const { 
        return m_errorCode; 
    }
    
    const std::string& message() const { 
        return m_errorMessage; 
    }

private:
    std::variant<T, std::monostate> m_value;
    int m_errorCode = 0;
    std::string m_errorMessage;
};

template<>
class Result<void> {
public:
    static Result ok() {
        Result r;
        r.m_success = true;
        return r;
    }

    static Result fail(int code, const std::string& message) {
        Result r;
        r.m_success = false;
        r.m_errorCode = code;
        r.m_errorMessage = message;
        return r;
    }

    bool isOk() const { 
        return m_success; 
    }

    int code() const { 
        return m_errorCode; 
    }
    
    const std::string& message() const { 
        return m_errorMessage; 
    }

private:
    bool m_success = false;
    int m_errorCode = 0;
    std::string m_errorMessage;
};

using VoidResult = Result<void>;

} // namespace chatroom
