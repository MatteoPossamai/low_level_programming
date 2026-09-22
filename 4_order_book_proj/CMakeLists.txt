cmake_minimum_required(VERSION 3.15...4.4)

project(OrderBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(benchmark REQUIRED)

add_library(sorted_vector_book STATIC
    data_structure/sorted_vector_str.cpp
)
target_include_directories(sorted_vector_book PUBLIC data_structure)
target_compile_options(sorted_vector_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_library(list_book STATIC
    data_structure/list_str.cpp
)
target_include_directories(list_book PUBLIC data_structure)
target_compile_options(list_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_library(tick_offset_book STATIC
    data_structure/tick_offset_str.cpp
)
target_include_directories(tick_offset_book PUBLIC data_structure)
target_compile_options(tick_offset_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_executable(bench_order_book benchmark/workload_bench.cpp)
target_link_libraries(bench_order_book PRIVATE
    sorted_vector_book
    list_book
    tick_offset_book
    benchmark::benchmark
)
target_compile_options(bench_order_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_library(protocol_codec STATIC
    protocol/message.cpp
)
target_include_directories(protocol_codec PUBLIC protocol)
target_compile_options(protocol_codec PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_executable(bench_codec benchmark/codec_bench.cpp)
target_link_libraries(bench_codec PRIVATE
    protocol_codec
    benchmark::benchmark
)
target_compile_options(bench_codec PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)
