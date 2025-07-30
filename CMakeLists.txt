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

# Find required packages
find_package(CURL)

# Static library
add_library(goldenhash STATIC src/goldenhash.cpp src/goldenhash_cipher.cpp src/goldenhash_sentiment.cpp)
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
target_link_libraries(goldenhash_test PRIVATE goldenhash m)

# Cipher test executable
add_executable(cipher_test src/cipher_test.cpp)
target_link_libraries(cipher_test PRIVATE goldenhash m)

# Sentiment demo executable
add_executable(sentiment_demo src/sentiment_demo.cpp)
target_link_libraries(sentiment_demo PRIVATE goldenhash m)

# Ollama support (optional)
if(CURL_FOUND)
    # Static library with Ollama support
    add_library(goldenhash_ollama STATIC src/goldenhash_ollama.cpp)
    target_link_libraries(goldenhash_ollama PUBLIC goldenhash ${CURL_LIBRARIES})
    target_include_directories(goldenhash_ollama PUBLIC ${CURL_INCLUDE_DIRS})
    
    # Ollama demo executable
    add_executable(ollama_demo src/ollama_demo.cpp)
    target_link_libraries(ollama_demo PRIVATE goldenhash_ollama goldenhash ${CURL_LIBRARIES} m)
    
    # Small Ollama demo
    add_executable(ollama_demo_small src/ollama_demo_small.cpp)
    target_link_libraries(ollama_demo_small PRIVATE goldenhash_ollama goldenhash ${CURL_LIBRARIES} m)
    
    message(STATUS "CURL found - Ollama support enabled")
else()
    message(STATUS "CURL not found - Ollama support disabled")
endif()
