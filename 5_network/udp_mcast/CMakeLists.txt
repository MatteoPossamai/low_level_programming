cmake_minimum_required(VERSION 3.15...4.4)

project(UdpMcast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(publisher publisher.cpp)
add_executable(subscriber subscriber.cpp)

target_compile_options(publisher PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

target_compile_options(subscriber PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)
