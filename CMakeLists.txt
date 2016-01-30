cmake_minimum_required(VERSION 2.8)

project(luna++)

find_package(Lua 5.2 REQUIRED)
include_directories("${LUA_INCLUDE_DIR}")

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-unused-parameter")

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

set(CMAKE_LD_FLAGS "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(libircclient/)
add_subdirectory(libmond/)
add_subdirectory(src/)

