cmake_minimum_required(VERSION 3.22)
project(chatroom-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(BUILD_TESTS "Build unit tests" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Protobuf REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/proto
    ${SPDLOG_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${NLOHMANN_JSON_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    /usr/include/mysql
    /usr/include/hiredis
)

add_library(chatroom_common STATIC
    src/common/logging.cpp
    src/util/config_manager.cpp
    src/util/utils.cpp
    src/util/thread_pool.cpp
    src/util/timer_wheel.cpp
    src/protocol/message_header.cpp
    src/protocol/message_codec.cpp
    src/protocol/message_dispatcher.cpp
    src/net/connection.cpp
    src/net/epoll_server.cpp
    src/data/mysql_pool.cpp
    src/data/redis_pool.cpp
    src/service/danmaku_service.cpp
    src/service/filter_service.cpp
)

target_link_libraries(chatroom_common PUBLIC
    spdlog::spdlog
    fmt::fmt
    ${PROTOBUF_LIBRARIES}
    nlohmann_json::nlohmann_json
    yaml-cpp
    mysqlclient
    hiredis
    pthread
)

target_compile_options(chatroom_common PRIVATE
    $<$<CONFIG:Debug>:-g -Wall -Wextra -Wpedantic>
    $<$<CONFIG:Release>:-O2 -DNDEBUG -Wall -Wextra -Wpedantic>
)

add_executable(chatroom-server
    src/main.cpp
)

target_link_libraries(chatroom-server PRIVATE chatroom_common)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    add_executable(chatroom-tests
        tests/test_main.cpp
        tests/test_result.cpp
        tests/test_config.cpp
        tests/test_thread_pool.cpp
        tests/test_timer_wheel.cpp
        tests/test_connection.cpp
        tests/test_protocol.cpp
        tests/test_filter.cpp
    )
    
    target_link_libraries(chatroom-tests PRIVATE
        chatroom_common
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(chatroom-tests)
endif()

install(TARGETS chatroom-server RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/chatroom-server)
