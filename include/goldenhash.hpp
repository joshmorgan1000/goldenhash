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
    GoldenHash(uint64_t table_size, uint64_t seed = 0) : N(table_size), seed_(seed) {
        // Find golden ratio primes
        uint64_t target_high = N / GOLDEN_RATIO;
        uint64_t target_low = N / (GOLDEN_RATIO * GOLDEN_RATIO);
        
        // Find nearest primes
        prime_high = find_nearest_prime(target_high);
        prime_low = find_nearest_prime(target_low);
        
        // Determine working modulus
        if (is_prime(N)) {
            // For prime N, we have options:
            // 1. Use N-1 (Fermat's little theorem) - causes issues with consecutive primes
            // 2. Use next composite number
            // 3. Use N+1 
            // For now, use N+1 to avoid collision issues
            working_mod = N + 1;
        } else {
            // For composite N, use N
            working_mod = N;
        }
        
        // Factorize for mixed-radix if needed
        factors = factorize(working_mod);
        
        // Initialize secret for 3 complete rotations
        // For modular arithmetic, we want values that cycle through the space
        size_t secret_size = 24;  // Keep same structure as XXH3
        secret.resize(secret_size);
        
        // Generate secret values using golden ratio mixing
        uint64_t h = N;
        for (size_t i = 0; i < secret_size; i++) {
            h = h * prime_high + i;
            h = (h + h / 33) % working_mod;
            h = (h * prime_low) % working_mod;
            h = (h + h / 29) % working_mod;
            // Already in modular space
            secret[i] = h;
        }
    }
    
    // Original hash method
    uint64_t hash(const uint8_t* data, size_t len) const {
        // Create chaos factor that incorporates table size
        uint64_t chaos = 0x5851f42d4c957f2dULL ^ (N * 0x9e3779b97f4a7c15ULL);
        uint64_t h = seed_ ^ chaos;
        
        // Process each byte with secret mixing - NO MODULOS in the loop
        for (size_t i = 0; i < len; i++) {
            // Mix with secret value (3 rotations through 24 values)
            uint64_t secret_val = secret[i % secret.size()];
            
            // Input mixing with XOR and multiplication
            h ^= (data[i] + secret_val) * prime_low;
            h *= prime_high;
            
            // Additional mixing with position-dependent value
            h ^= h >> 33;
            h *= (prime_high + i * secret_val);
            h ^= h >> 29;
        }
        
        // Final avalanche mixing - still no modulos
        // Mix in table size again to ensure it affects the distribution
        h ^= N * 0x165667919E3779F9ULL;  // XXH3 avalanche prime
        
        // Standard integer hash avalanche
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        
        // Mix in length for additional entropy
        h ^= len * prime_low;
        
        // SINGLE modulo at the very end
        return h % N;
    }
    
    void print_info() const {
        std::cout << "Table size (N): " << N << "\n";
        std::cout << "Is prime: " << (is_prime(N) ? "Yes" : "No") << "\n";
        std::cout << "Working modulus: " << working_mod << "\n";
        std::cout << "High prime (N/φ): " << prime_high << " (target: " << uint64_t(N/GOLDEN_RATIO) << ")\n";
        std::cout << "Low prime (N/φ²): " << prime_low << " (target: " << uint64_t(N/(GOLDEN_RATIO*GOLDEN_RATIO)) << ")\n";
        std::cout << "Factorization: ";
        for (auto f : factors) std::cout << f << " ";
        std::cout << "\n";
        std::cout << "Golden ratio check: N/prime_high = " << double(N)/prime_high << " (φ = " << GOLDEN_RATIO << ")\n";
    }
    
    uint64_t get_table_size() const { return N; }
    uint64_t get_prime_high() const { return prime_high; }
    uint64_t get_prime_low() const { return prime_low; }
    uint64_t get_working_mod() const { return working_mod; }
    const std::vector<uint64_t>& get_factors() const { return factors; }

private:
    uint64_t N;              // Table size
    uint64_t prime_high;     // Prime near N/φ
    uint64_t prime_low;      // Prime near N/φ²
    uint64_t working_mod;    // Modulus for operations
    std::vector<uint64_t> factors;
    std::vector<uint64_t> secret;  // Modular secret for mixing
    uint64_t seed_;          // Seed value

    // Simple primality test
    bool is_prime(uint64_t n) const {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (uint64_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    // Find factors of N for mixed-radix representation
    std::vector<uint64_t> factorize(uint64_t n) {
        std::vector<uint64_t> factors;
        uint64_t temp = n;
        
        for (uint64_t i = 2; i * i <= temp; i++) {
            while (temp % i == 0) {
                factors.push_back(i);
                temp /= i;
            }
        }
        if (temp > 1) factors.push_back(temp);
        
        return factors;
    }

    uint64_t find_nearest_prime(uint64_t target) const {
        // Search outward from target
        for (uint64_t delta = 0; delta < 1000; delta++) {
            if (target > delta && is_prime(target - delta)) return target - delta;
            if (is_prime(target + delta)) return target + delta;
        }
        return target; // Fallback
    }
};

} 
