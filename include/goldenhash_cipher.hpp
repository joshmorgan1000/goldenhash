/**
 * Copyright 2025 Josh Morgan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
/**
 * @file goldenhash_cipher.hpp
 * @brief Experimental stream cipher using GoldenHash as building blocks
 * @details This file implements an experimental cipher using 4 chained GoldenHash
 * instances with 16-bit keys (11 bits table size, 5 bits seed)
 */

#include "goldenhash.hpp"
#include <array>
#include <vector>
#include <cstring>

namespace goldenhash {

/**
 * @class GoldenHashCipher
 * @brief Experimental stream cipher using chained GoldenHash instances
 * 
 * This class implements a stream cipher using 4 GoldenHash instances, each
 * configured with a 16-bit key (11 bits for table size, 5 bits for seed).
 * The 2048 possible table sizes are selected from prime-dense regions.
 */
class GoldenHashCipher {
public:
    static constexpr size_t NUM_CIPHERS = 4;
    static constexpr size_t KEY_SIZE = 8; // 4 * 16 bits = 8 bytes
    static constexpr size_t TABLE_SIZE_BITS = 11;
    static constexpr size_t SEED_BITS = 5;
    static constexpr size_t MAX_TABLE_INDEX = (1 << TABLE_SIZE_BITS) - 1; // 2047
    static constexpr size_t MAX_SEED = (1 << SEED_BITS) - 1; // 31

    /**
     * @brief Constructor that initializes with an 8-byte key
     * @param key 8-byte key (4 * 16-bit subkeys)
     */
    GoldenHashCipher(const uint8_t key[KEY_SIZE]);

    /**
     * @brief Encrypt/decrypt data (stream cipher - same operation)
     * @param data Data to process
     * @param len Length of data
     * @param output Output buffer (must be at least len bytes)
     */
    void process(const uint8_t* data, size_t len, uint8_t* output);

    /**
     * @brief Get the prime table sizes used for cipher instances
     * @return Vector of 2048 prime-dense table sizes
     */
    static const std::vector<uint64_t>& get_prime_table_sizes();

    /**
     * @brief Get table size for a given 11-bit index
     * @param index 11-bit index (0-2047)
     * @return Table size from prime-dense region
     */
    static uint64_t get_table_size(size_t index);

    /**
     * @brief Analyze key properties
     * @param key 8-byte key to analyze
     */
    static void analyze_key(const uint8_t key[KEY_SIZE]);

private:
    std::array<std::unique_ptr<GoldenHash>, NUM_CIPHERS> hashers;
    uint64_t counter;
    
    /**
     * @brief Initialize prime table sizes (called once)
     */
    static void init_prime_tables();
    
    static std::vector<uint64_t> prime_table_sizes;
    static bool tables_initialized;
};

/**
 * @class CipherAnalyzer
 * @brief Analysis tools for GoldenHashCipher
 */
class CipherAnalyzer {
public:
    /**
     * @brief Test avalanche effect - how bit changes propagate
     * @param num_tests Number of test iterations
     */
    static void test_avalanche(size_t num_tests = 10000);
    
    /**
     * @brief Test diffusion - how input changes affect output
     * @param num_tests Number of test iterations
     */
    static void test_diffusion(size_t num_tests = 10000);
    
    /**
     * @brief Test for patterns in keystream
     * @param key_bytes Key to test
     * @param stream_length Length of keystream to analyze
     */
    static void test_keystream_patterns(const uint8_t key[8], size_t stream_length = 1048576);
    
    /**
     * @brief Test correlation between different keys
     * @param num_keys Number of random keys to test
     */
    static void test_key_correlation(size_t num_keys = 1000);
    
    /**
     * @brief Run all analysis tests
     */
    static void run_all_tests();
};

} // namespace goldenhash