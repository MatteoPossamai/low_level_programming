cmake_minimum_required(VERSION 3.15...4.4)

project(Poll_Interfaces LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(epoll_server epoll_server.cpp)
target_compile_options(epoll_server PRIVATE -O2 -Wall -Wextra -Wshadow)

add_executable(io_uring_server io_uring_server.cpp)
target_compile_options(io_uring_server PRIVATE -O2 -Wall -Wextra -D_GLIBCXX_ASSERTIONS)
target_link_libraries(io_uring_server PRIVATE uring)
