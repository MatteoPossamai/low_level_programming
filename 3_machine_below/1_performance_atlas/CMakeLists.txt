cmake_minimum_required(VERSION 3.15...4.4)

project(Performance_Atlas LANGUAGES CXX)

add_executable(1_b_miss 1_branch_missprediction.cpp)
find_package(benchmark REQUIRED)
target_link_libraries(1_b_miss PRIVATE benchmark::benchmark)
