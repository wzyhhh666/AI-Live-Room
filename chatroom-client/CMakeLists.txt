cmake_minimum_required(VERSION 3.16)
project(chatroom-client VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

add_executable(chatroom-client
    src/main.cpp
    src/connection.cpp
    src/protocol.cpp
    src/client.cpp
)

target_include_directories(chatroom-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/common
)

if(UNIX)
    target_link_libraries(chatroom-client PRIVATE pthread)
elseif(WIN32)
    target_link_libraries(chatroom-client PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(chatroom-client PRIVATE /W3 /utf-8)
else()
    target_compile_options(chatroom-client PRIVATE -Wall -Wextra -Wpedantic)
endif()
