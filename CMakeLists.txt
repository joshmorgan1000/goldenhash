cmake_minimum_required(VERSION 3.20)
project(goldenhash VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directories
include_directories(include)

# Build flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")

# Find required packages
find_package(CURL)
find_package(SQLite3)
find_package(OpenSSL REQUIRED)

# Add xxHash from third_party
add_library(xxhash STATIC third_party/xxHash/xxhash.c)
target_include_directories(xxhash PUBLIC third_party/xxHash)

# Static library
add_library(goldenhash STATIC src/goldenhash.cpp)
target_include_directories(goldenhash PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(goldenhash PUBLIC cxx_std_23)

# Set library properties
set_target_properties(goldenhash PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    POSITION_INDEPENDENT_CODE ON
)

# Main executable
add_executable(goldenhash_test src/main.cpp)
target_link_libraries(goldenhash_test PRIVATE goldenhash m xxhash SQLite::SQLite3 OpenSSL::Crypto)
target_compile_definitions(goldenhash_test PRIVATE HAVE_SQLITE3)

# S-box analysis executable
add_executable(sbox_test src/sbox_test.cpp)
target_link_libraries(sbox_test PRIVATE goldenhash)
