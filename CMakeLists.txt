cmake_minimum_required(VERSION 3.20)

project(veloce LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(veloce INTERFACE)
target_compile_features(veloce INTERFACE cxx_std_23)
target_include_directories(
    veloce
    INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_sources(
    veloce
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/parallel.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/reduce.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/thread_pool.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/chunker.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/concepts.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/definitions.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/veloce/transform_reduce.hpp"
)

add_executable(veloce_parallel_bench bench/parallel.cpp)
target_link_libraries(veloce_parallel_bench PRIVATE veloce)

add_executable(veloce_reduce_bench bench/reduce.cpp)
target_link_libraries(veloce_reduce_bench PRIVATE veloce)