cmake_minimum_required(VERSION 3.15...4.4)

project(Performance_Atlas LANGUAGES CXX)

add_executable(1_b_miss 1_branch_missprediction.cpp)
add_executable(2_c_v_b 2_cmov_vs_branch.cpp)
add_executable(3_acc 3_accumulators.cpp)
add_executable(4_stride 4_stride.cpp)
add_executable(5_fs 5_false_sharing.cpp)
add_executable(6_pf 6_prefetching.cpp)

target_compile_options(1_b_miss PRIVATE
    -O2
)

target_compile_options(2_c_v_b PRIVATE
    -O2
    -fno-if-conversion
    -fno-if-conversion2
)

target_compile_options(3_acc PRIVATE
    -O2
)

target_compile_options(4_stride PRIVATE
    -O2
)


target_compile_options(5_fs PRIVATE
    -O2 -pthread
)

target_compile_options(6_pf PRIVATE
    -O2
)


find_package(benchmark REQUIRED)
target_link_libraries(1_b_miss PRIVATE benchmark::benchmark)
target_link_libraries(2_c_v_b  PRIVATE benchmark::benchmark)
target_link_libraries(3_acc PRIVATE benchmark::benchmark)
target_link_libraries(4_stride PRIVATE benchmark::benchmark)
target_link_libraries(5_fs PRIVATE benchmark::benchmark)
target_link_libraries(6_pf PRIVATE benchmark::benchmark)
