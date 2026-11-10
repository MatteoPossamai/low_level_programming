cmake_minimum_required(VERSION 3.15...4.4)

project(FoundationsTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(benchmark REQUIRED)

add_executable(bench_conv bench_conv.cpp stb_impl.cpp)
target_link_libraries(bench_conv PRIVATE
    benchmark::benchmark
)

target_compile_options(bench_conv PRIVATE
    -O2 -march=native -g -Wall -Wextra -Wshadow -std=c++20
)

# Keep optimization remarks focused on the benchmark source. stb's
# implementation is compiled separately in stb_impl.cpp.
set_source_files_properties(bench_conv.cpp PROPERTIES
    COMPILE_OPTIONS "-Rpass=loop-vectorize"
)

add_executable(test_1 test_1.cpp)
target_compile_options(test_1 PRIVATE
  -g -Wall -Wextra -Wshadow
  -std=c++20 -g -O2 -march=native
  -Rpass=loop-vectorize
  -Rpass-missed=loop-vectorize
  -Rpass-analysis=loop-vectorize
)

add_executable(test test.cpp)
target_compile_options(test PRIVATE
  -g -Wall -Wextra -Wshadow
  -std=c++20 -g -O2 -mavx
  -Rpass=loop-vectorize
  -Rpass-missed=loop-vectorize
  -Rpass-analysis=loop-vectorize
)
