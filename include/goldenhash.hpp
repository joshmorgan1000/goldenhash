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
 * @file goldenhash.hpp
 * @brief Header file for the GoldenHash golden ratio hash function
 * @details This file defines the GoldenHash class which implements a hash function
 * based on primes and the golden ratio. It aims to efficiently generate a high
 * performance hash function for any N size hash table. We expect it to hit all the
 * metrics - Chi-square, collision rate, avalanche effect, and the birthday paradox.
 */
#include <chrono>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <vector>
#include <string>
#include <span>
#include <iostream>

namespace goldenhash {

static const long double GOLDEN_RATIO = (1 + std::sqrt(5)) / 2; // Golden ratio constant

#ifdef __has_include
#  if __has_include(<json/json.h>)
#    include <json/json.h>
#    define HAS_JSONCPP 1
#  else
#    define HAS_JSONCPP 0
#  endif
#else
#  define HAS_JSONCPP 0
#endif

/**
 * @struct MetricResult
 * @brief Container for individual metric results
 */
struct MetricResult {
    std::string name;
    double value;
    std::string unit;
    std::string description;
    std::map<std::string, double> details;
};

/**
 * @struct TestConfiguration
 * @brief Configuration for hash function tests
 */
struct TestConfiguration {
    uint64_t num_keys = 1000000;
    uint64_t table_size = 1024;
    uint64_t seed = 0;
    bool test_avalanche = true;
    bool test_distribution = true;
    bool test_collisions = true;
    bool test_performance = true;
    bool test_bit_independence = true;
    uint32_t num_performance_runs = 10;
    uint32_t avalanche_samples = 10000;
};

/**
 * @class GoldenHash
 * @brief Implementation of a modular golden ratio hash function
 * 
 * This class implements a hash function based on the golden ratio and prime numbers.
 * It aims to provide a high-performance hash function suitable for in-memory hash tables.
 * It can hash based on any arbitrary table size N, using primes near N/φ and N/φ².
 */
class GoldenHash {    
public:
    /**
     * @brief Constructor for GoldenHash
     * @param table_size Size of the hash table
     * @param seed Seed value for the hash function (default: 0)
     */
    GoldenHash(uint64_t table_size, uint64_t seed = 0);
    
    /**
     * @brief Hash function
     * @param data Pointer to data to hash
     * @param len Length of data in bytes
     * @return Hash value in range [0, N)
     */
    uint64_t hash(const uint8_t* data, size_t len) const;
    
    /**
     * @brief Print information about the hash function configuration
     */
    void print_info() const;
    
    /**
     * @brief Get the table size
     * @return Table size N
     */
    uint64_t get_table_size() const;
    
    /**
     * @brief Get the high prime value
     * @return Prime near N/φ
     */
    uint64_t get_prime_high() const;
    
    /**
     * @brief Get the low prime value  
     * @return Prime near N/φ²
     */
    uint64_t get_prime_low() const;
    
    /**
     * @brief Get the working modulus
     * @return Working modulus for operations
     */
    uint64_t get_working_mod() const;
    
    /**
     * @brief Get the factorization of the working modulus
     * @return Vector of factors
     */
    const std::vector<uint64_t>& get_factors() const;

private:
    uint64_t N;              // Table size
    uint64_t prime_high;     // Prime near N/φ
    uint64_t prime_low;      // Prime near N/φ²
    uint64_t working_mod;    // Modulus for operations
    std::vector<uint64_t> factors;
    std::vector<uint64_t> secret;  // Modular secret for mixing
    uint64_t seed_;          // Seed value

    /**
     * @brief Simple primality test
     * @param n Number to test
     * @return True if n is prime, false otherwise
     */
    bool is_prime(uint64_t n) const;

    /**
     * @brief Find factors of N for mixed-radix representation
     * @param n Number to factorize
     * @return Vector of prime factors
     */
    std::vector<uint64_t> factorize(uint64_t n);

    /**
     * @brief Find the nearest prime to a target value
     * @param target Target value to find prime near
     * @return Nearest prime number
     */
    uint64_t find_nearest_prime(uint64_t target) const;
};

} 
