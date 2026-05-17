cmake_minimum_required(VERSION 3.20)
project(chatroom-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W4 /EHsc")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

option(BUILD_TESTS "Build unit tests" ON)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

find_package(spdlog CONFIG QUIET)
find_package(fmt CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)

if(NOT spdlog_FOUND)
    find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h PATHS
        ${THIRD_PARTY_DIR}/spdlog/include
        NO_DEFAULT_PATH)
endif()

if(NOT fmt_FOUND)
    find_path(FMT_INCLUDE_DIR NAMES fmt/format.h PATHS
        ${THIRD_PARTY_DIR}/fmt-header-only/include
        NO_DEFAULT_PATH)
endif()

if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR NAMES nlohmann/json.hpp PATHS
        ${THIRD_PARTY_DIR}/nlohmann-json
        ${THIRD_PARTY_DIR}/nlohmann-json/nlohmann
        NO_DEFAULT_PATH)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

add_library(chatroom_common STATIC
    src/common/result.cpp
    src/common/logger.cpp
)

target_include_directories(chatroom_common PUBLIC
    ${SPDLOG_INCLUDE_DIR}
    ${FMT_INCLUDE_DIR}
    ${NLOHMANN_JSON_INCLUDE_DIR}
)

if(TARGET spdlog::spdlog)
    target_link_libraries(chatroom_common PUBLIC spdlog::spdlog)
elseif(SPDLOG_INCLUDE_DIR)
    target_compile_definitions(chatroom_common PUBLIC SPDLOG_HEADER_ONLY)
endif()

if(TARGET fmt::fmt)
    target_link_libraries(chatroom_common PUBLIC fmt::fmt-header-only)
endif()

file(GLOB PROTOCOL_SOURCES src/protocol/*.cpp)
file(GLOB NET_SOURCES src/net/*.cpp)
file(GLOB SERVICE_SOURCES src/service/*.cpp)
file(GLOB DATA_SOURCES src/data/*.cpp)

if(PROTOCOL_SOURCES)
    add_library(chatroom_protocol STATIC ${PROTOCOL_SOURCES})
    target_include_directories(chatroom_protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(chatroom_protocol PUBLIC chatroom_common)
endif()

if(NET_SOURCES)
    add_library(chatroom_net STATIC ${NET_SOURCES})
    if(WIN32)
        target_link_libraries(chatroom_net PUBLIC chatroom_protocol ws2_32)
    else()
        target_link_libraries(chatroom_net PUBLIC chatroom_protocol pthread)
    endif()
endif()

if(SERVICE_SOURCES)
    add_library(chatroom_service STATIC ${SERVICE_SOURCES})
    target_link_libraries(chatroom_service PUBLIC chatroom_net)
endif()

if(DATA_SOURCES)
    add_library(chatroom_data STATIC ${DATA_SOURCES})
    find_package(Threads QUIET)
    if(Threads_FOUND)
        target_link_libraries(chatroom_data PUBLIC Threads::Threads)
    endif()
endif()

add_executable(chatroom_server
    src/main.cpp
)

target_link_libraries(chatroom_server PRIVATE chatroom_common)
if(TARGET chatroom_protocol)
    target_link_libraries(chatroom_server PRIVATE chatroom_protocol)
endif()
if(TARGET chatroom_net)
    target_link_libraries(chatroom_server PRIVATE chatroom_net)
endif()
if(TARGET chatroom_service)
    target_link_libraries(chatroom_server PRIVATE chatroom_service)
endif()
if(TARGET chatroom_data)
    target_link_libraries(chatroom_server PRIVATE chatroom_data)
endif()

message(STATUS "")
message(STATUS "========== ChatRoom Server ==========")
message(STATUS "  Platform:         ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C++ Standard:     C++17")
message(STATUS "  Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  spdlog:           ${SPDLOG_INCLUDE_DIR}")
message(STATUS "  fmt:              ${FMT_INCLUDE_DIR}")
message(STATUS "  nlohmann_json:    ${NLOHMANN_JSON_INCLUDE_DIR}")
message(STATUS "=======================================")
message(STATUS "")
