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
#include <cstdlib>
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
#include <unordered_map>

namespace goldenhash {

static const long double GOLDEN_RATIO = (1 + std::sqrt(5)) / 2;
static const long double GOLDEN_FRACTIONAL_PART = (std::sqrt(5) - 1) / 2;

/**
 * @brief Generate a random large prime number
 * 
 * This function generates a random large prime number suitable for use in the hash function.
 * It uses a random number generator and checks for primality.
 * 
 * @return A random large prime number
 */
inline static uint64_t random_large_prime() {
    // Generate a random large prime number
    std::cout << "Generating random large prime...\n";
    std::mt19937_64 rng(std::random_device{}());
    uint8_t shift_mod = rand() % 5;
    std::uniform_int_distribution<uint64_t> dist(1ULL << (33 + shift_mod), UINT64_MAX);
    uint64_t candidate;
    do {
        candidate = dist(rng);
        // Check if the candidate is prime
        if (candidate % 2 == 0) continue; // Skip even numbers
        bool is_prime = true;
        for (uint64_t i = 3; i * i <= candidate; i += 2) {
            if (candidate % i == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) return candidate;
    } while (true);
}

/**
 * @class GoldenHash
 * @brief Implementation of a modular golden ratio hash function
 * 
 * This class implements a hash function based on the golden ratio and prime numbers.
 * It aims to provide a high-performance hash function suitable for in-memory hash tables.
 * It can hash based on any arbitrary table size N, using primes near N/φ and N/φ².
 */
class GoldenHash128 {    
public:
    /**
     * @brief Constructor for GoldenHash
     * @param table_size Size of the hash table
     * @param seed Seed value for the hash function (default: 0)
     */
    GoldenHash128(__uint128_t table_size, uint64_t seed = 0) : N(table_size), seed_(seed) {
        // Find golden ratio primes
        __uint128_t target_high = N / GOLDEN_RATIO;
        __uint128_t target_low = N / (GOLDEN_RATIO * GOLDEN_RATIO);
        
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
        size_t secret_size = 24;  // Keep same structure as XXH3
        secret.resize(secret_size);
        
        // Generate secret values using golden ratio mixing
        __uint128_t h = N;
        for (size_t i = 0; i < secret_size; i++) {
            h = h * prime_high + i;
            h = (h + h / 33) % working_mod;
            h = (h * prime_low) % working_mod;
            h = (h + h / 29) % working_mod;
            // Already in modular space
            secret[i] = h;
        }
    }
    
    /**
     * @brief Hash function
     * @param data Pointer to data to hash
     * @param len Length of data in bytes
     * @return Hash value in range [0, N)
     */
    __uint128_t hash(const uint8_t* data, size_t len) const {
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
    
    /**
     * @brief Print information about the hash function configuration
     */
    void print_info() const {
        std::cout << "GoldenHash Configuration:\n";
        std::cout << "  Table Size: " << uint128_to_string(N) << "\n";
        std::cout << "  Prime High: " << uint128_to_string(prime_high) << "\n";
        std::cout << "  Prime Low: " << uint128_to_string(prime_low) << "\n";
        std::cout << "  Working Modulus: " << uint128_to_string(working_mod) << "\n";
        std::cout << "  Seed: " << seed_ << "\n";
        std::cout << "  Factors: ";
        for (const auto& factor : factors) {
            std::cout << uint128_to_string(factor) << " ";
        }
        std::cout << "\n";
    }
    
    /**
     * @brief Get the table size
     * @return Table size N
     */
    __uint128_t get_table_size() const {
        return N;
    }
    
    /**
     * @brief Get the high prime value
     * @return Prime near N/φ
     */
    __uint128_t get_prime_high() const {
        return prime_high;
    }
    
    /**
     * @brief Get the low prime value  
     * @return Prime near N/φ²
     */
    __uint128_t get_prime_low() const {
        return prime_low;
    }
    
    /**
     * @brief Get the working modulus
     * @return Working modulus for operations
     */
    __uint128_t get_working_mod() const {
        return working_mod;
    }
    
    /**
     * @brief Get the factorization of the working modulus
     * @return Vector of factors
     */
    const std::vector<__uint128_t>& get_factors() const {
        return factors;
    }

private:
    __uint128_t N;              // Table size
    __uint128_t prime_high;     // Prime near N/φ
    __uint128_t prime_low;      // Prime near N/φ²
    __uint128_t working_mod;    // Modulus for operations
    std::vector<__uint128_t> factors;
    std::vector<__uint128_t> secret;  // Modular secret for mixing
    uint64_t seed_;          // Seed value

    /**
     * @brief Convert __uint128_t to string
     * @param value Value to convert
     * @return String representation of the value
     */
    std::string uint128_to_string(__uint128_t value) const {
        if (value == 0) return "0";
        
        std::string result;
        while (value > 0) {
            result = char('0' + value % 10) + result;
            value /= 10;
        }
        return result;
    }

    /**
     * @brief Simple primality test
     * @param n Number to test
     * @return True if n is prime, false otherwise
     */
    bool is_prime(__uint128_t n) const {
        if (n < 2) return false;
        if (n == 2 || n == 3) return true;
        if (n % 2 == 0) return false;
        for (__uint128_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }

    /**
     * @brief Find factors of N for mixed-radix representation
     * @param n Number to factorize
     * @return Vector of prime factors
     */
    std::vector<__uint128_t> factorize(__uint128_t n) {
        std::vector<__uint128_t> factors;
        for (__uint128_t i = 2; i * i <= n; i++) {
            while (n % i == 0) {
                factors.push_back(i);
                n /= i;
            }
        }
        if (n > 1) {
            factors.push_back(n); // Remaining prime factor
        }
        return factors;
    }

    bool miller_rabin(__uint128_t n, int k = 8) const {
        if (n < 4) return n == 2 || n == 3;
        if (n % 2 == 0) return false;
        __uint128_t d = n - 1;
        int r = 0;
        while ((d & 1) == 0) { d >>= 1; r++; }

        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist(2, (uint64_t)std::min(n-2, (__uint128_t)UINT64_MAX));

        for (int i = 0; i < k; i++) {
            uint64_t a = dist(rng);
            __uint128_t x = 1, base = a, exp = d, mod = n;
            // Compute x = a^d % n
            while (exp) {
                if (exp & 1) x = (x * base) % mod;
                base = (base * base) % mod;
                exp >>= 1;
            }
            if (x == 1 || x == n - 1) continue;
            bool continue_outer = false;
            for (int j = 0; j < r - 1; j++) {
                x = (x * x) % n;
                if (x == n - 1) { continue_outer = true; break; }
            }
            if (continue_outer) continue;
            return false;
        }
        return true; // Probably prime!
    }

    /**
     * @brief Find the nearest prime to a target value
     * @param target Target value to find prime near
     * @return Nearest prime number
     */
    __uint128_t find_nearest_prime(__uint128_t target) {
        if (target % 2 == 0) target++;
        while (true) {
            if (miller_rabin(target, 16)) return target;
            target += 2; // Check next odd number
        }
    }
};

} 
