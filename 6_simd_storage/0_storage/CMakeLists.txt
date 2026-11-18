cmake_minimum_required(VERSION 3.15...4.4)

project(SDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(bitcask_main src/bitcask_main.cpp)
target_compile_options(bitcask_main PRIVATE
  -O2 -g -march=native -Wall
  -Wextra -Wpedantic -Wconversion
)
