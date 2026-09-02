cmake_minimum_required(VERSION 3.15...4.4)

project(Performance_Atlas LANGUAGES CXX)

add_executable(1_b_miss 1_branch_missprediction.cpp)
add_executable(2_c_v_b 2_cmov_vs_branch.cpp)

target_compile_options(2_c_v_b PRIVATE
    -O2
    -fno-if-conversion
    -fno-if-conversion2
)

find_package(benchmark REQUIRED)
target_link_libraries(1_b_miss PRIVATE benchmark::benchmark)
target_link_libraries(2_c_v_b  PRIVATE benchmark::benchmark)
