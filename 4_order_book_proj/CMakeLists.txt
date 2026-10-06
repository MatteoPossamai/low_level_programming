cmake_minimum_required(VERSION 3.15...4.4)

project(OrderBook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

add_library(sorted_vector_book STATIC
    data_structure_bench/sorted_vector_str.cpp
)
target_include_directories(sorted_vector_book PUBLIC data_structure_bench)
target_compile_options(sorted_vector_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_library(list_book STATIC
    data_structure_bench/list_str.cpp
)
target_include_directories(list_book PUBLIC data_structure_bench)
target_compile_options(list_book PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_library(tick_offset_book STATIC
    data_structure_bench/tick_offset_str.cpp
)
target_include_directories(tick_offset_book PUBLIC data_structure_bench)
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

add_library(engine STATIC
    src/message.cpp
)
target_include_directories(engine PUBLIC src)
target_compile_options(engine PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_executable(bench_codec benchmark/codec_bench.cpp)
target_link_libraries(bench_codec PRIVATE
    engine
    benchmark::benchmark
)
target_compile_options(bench_codec PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_executable(allocator_tests tests/allocator_test.cpp)
target_include_directories(allocator_tests PRIVATE src)
target_link_libraries(allocator_tests PRIVATE GTest::gtest_main)
target_compile_options(allocator_tests PRIVATE
    -O0 -g -Wall -Wextra -Wshadow
)

set(SANITIZERS
    -fsanitize=address,undefined
    -fno-sanitize-recover=undefined
    -fno-omit-frame-pointer
)
target_compile_options(allocator_tests PRIVATE ${SANITIZERS})
target_link_options(allocator_tests PRIVATE ${SANITIZERS})

add_executable(engine_tests tests/engine_test.cpp src/message.cpp)
target_include_directories(engine_tests PRIVATE src)
target_link_libraries(engine_tests PRIVATE GTest::gtest_main)
target_compile_options(engine_tests PRIVATE
    -O1 -g -Wall -Wextra -Wshadow ${SANITIZERS}
)
target_link_options(engine_tests PRIVATE ${SANITIZERS})

add_executable(bench_engine benchmark/engine_bench.cpp)
target_link_libraries(bench_engine PRIVATE
    engine
    benchmark::benchmark
)
target_compile_options(bench_engine PRIVATE
    -O2 -g -Wall -Wextra -Wshadow
)

add_executable(network_device_tests tests/network_device_test.cpp)
target_include_directories(network_device_tests PRIVATE src)
target_link_libraries(network_device_tests PRIVATE GTest::gtest_main)
target_compile_options(network_device_tests PRIVATE
    -O0 -g -Wall -Wextra -Wshadow
)

add_executable(complete_flow_tests tests/test_complete_flow.cpp src/message.cpp)
target_include_directories(complete_flow_tests PRIVATE src)
target_link_libraries(complete_flow_tests PRIVATE GTest::gtest_main)
target_compile_options(complete_flow_tests PRIVATE
    -O1 -g -Wall -Wextra -Wshadow ${SANITIZERS}
)
target_link_options(complete_flow_tests PRIVATE ${SANITIZERS})

add_executable(order_flow_demo tests/complete_flow_demo.cpp)
target_include_directories(order_flow_demo PRIVATE src)
target_compile_options(order_flow_demo PRIVATE
    -O0 -g -Wall -Wextra -Wshadow
)

enable_testing()
add_test(NAME network_device_tests COMMAND network_device_tests)
set_tests_properties(network_device_tests PROPERTIES TIMEOUT 15)
add_test(NAME complete_flow_tests COMMAND complete_flow_tests)
set_tests_properties(complete_flow_tests PROPERTIES TIMEOUT 15)
add_executable(main src/main.cpp src/message.cpp)
target_compile_options(main PRIVATE
  -O2 -g -Wall -Wextra -Wshadow -fsanitize=thread
)
target_link_options(main PRIVATE -fsanitize=thread)
target_include_directories(main PRIVATE src)

find_package(PkgConfig)
target_link_libraries(main PRIVATE uring)
