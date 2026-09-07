cmake_minimum_required(VERSION 3.15...4.4)

project(Compiler_Atlas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(benchmark REQUIRED)

function(add_experiment name src)
    add_executable(${name} ${src})
    target_compile_options(${name} PRIVATE -O2 -Wall -Wextra -Wshadow ${ARGN})
    target_link_libraries(${name} PRIVATE benchmark::benchmark)

    # Assembly dump target: build with `cmake --build . --target ${name}_asm`
    add_custom_target(${name}_asm
        COMMAND ${CMAKE_CXX_COMPILER}
                -std=c++20 -O2 -S -fverbose-asm
                -masm=intel
                ${ARGN}
                ${CMAKE_CURRENT_SOURCE_DIR}/${src}
                -o ${CMAKE_CURRENT_BINARY_DIR}/${name}.s
        COMMENT "Generating ${name}.s (Intel syntax, verbose)"
        VERBATIM
    )
endfunction()

add_experiment(1_inlining        1_inlining.cpp)
add_experiment(2_ub_signed       2_ub_signed.cpp)
add_experiment(2_ub_signed_wrapv 2_ub_signed.cpp -fwrapv)
add_experiment(3_autovec         3_autovec.cpp -march=native -fopt-info-vec-all=vec.log)
add_experiment(4_noexcept        4_noexcept.cpp)
add_experiment(5_virtual_vs_crtp 5_virtual_vs_crtp.cpp -march=native)
add_experiment(6_exceptions      6_exceptions.cpp)
