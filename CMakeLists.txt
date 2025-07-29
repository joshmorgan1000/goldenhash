cmake_minimum_required(VERSION 3.20)
project(goldenhash VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directories
include_directories(include)

# Build flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")

# Library (header-only for now)
add_library(goldenhash INTERFACE)
target_include_directories(goldenhash INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Experimental programs
add_executable(test_float_hash experimental/test_float_hash.cpp)
target_link_libraries(test_float_hash PRIVATE m)

# Test programs
add_executable(test_float_hash_metrics tests/test_float_hash_metrics.cpp)
target_link_libraries(test_float_hash_metrics PRIVATE m)
