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

/**
 * @file goldenhash.cpp
 * @brief Implementation file for the GoldenHash golden ratio hash function
 * @details This file contains the implementation of the GoldenHash class
 */

#include "goldenhash.hpp"

namespace goldenhash {

/**
 * @brief Constructor for GoldenHash
 * @param table_size Size of the hash table
 * @param seed Seed value for the hash function
 */
GoldenHash::GoldenHash(uint64_t table_size, uint64_t seed) : N(table_size), seed_(seed) {
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

/**
 * @brief Hash function implementation
 * @param data Pointer to data to hash
 * @param len Length of data in bytes
 * @return Hash value in range [0, N)
 */
uint64_t GoldenHash::hash(const uint8_t* data, size_t len) const {
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
void GoldenHash::print_info() const {
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

/**
 * @brief Get the table size
 * @return Table size N
 */
uint64_t GoldenHash::get_table_size() const { return N; }

/**
 * @brief Get the high prime value
 * @return Prime near N/φ
 */
uint64_t GoldenHash::get_prime_high() const { return prime_high; }

/**
 * @brief Get the low prime value
 * @return Prime near N/φ²
 */
uint64_t GoldenHash::get_prime_low() const { return prime_low; }

/**
 * @brief Get the working modulus
 * @return Working modulus for operations
 */
uint64_t GoldenHash::get_working_mod() const { return working_mod; }

/**
 * @brief Get the factorization of the working modulus
 * @return Vector of factors
 */
const std::vector<uint64_t>& GoldenHash::get_factors() const { return factors; }

/**
 * @brief Simple primality test
 * @param n Number to test
 * @return True if n is prime, false otherwise
 */
bool GoldenHash::is_prime(uint64_t n) const {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (uint64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

/**
 * @brief Find factors of N for mixed-radix representation
 * @param n Number to factorize
 * @return Vector of prime factors
 */
std::vector<uint64_t> GoldenHash::factorize(uint64_t n) {
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

/**
 * @brief Find the nearest prime to a target value
 * @param target Target value to find prime near
 * @return Nearest prime number
 */
uint64_t GoldenHash::find_nearest_prime(uint64_t target) const {
    // Search outward from target
    for (uint64_t delta = 0; delta < 1000; delta++) {
        if (target > delta && is_prime(target - delta)) return target - delta;
        if (is_prime(target + delta)) return target + delta;
    }
    return target; // Fallback
}

} // namespace goldenhash