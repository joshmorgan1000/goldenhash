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
    
    // Factorize for mixed-radix if needed
    factors = factorize(N);
    
    // Initialize compressive S-boxes (12-bit to 8-bit)
    // 4KB per S-box fits in L1 cache
    for (size_t i = 0; i < NUM_SBOXES; i++) {
        sboxes[i] = new (std::align_val_t(64)) uint8_t[SBOX_SIZE];
    }
    
    // Generate S-box 1: compress 8 bits to 8 bits using golden ratio primes
    // Use mixed AND/OR operations with primes for better mixing
    prime_product = (prime_high * prime_low);
    prime_mod = prime_product % N;
    working_mod = (((N | prime_low) ^ (N & prime_high)) % (N << 4)) >> 4;
    prime_mixed = (prime_product * (1 / GOLDEN_RATIO));

    uint64_t h = ((N ^ prime_product) * seed_) | (seed_ & 0xFFF);
    h = (h * prime_product) ^ prime_mod;
    h = (h * prime_low) ^ (prime_high);
    h = (h & ~prime_mixed) | (((h ^ prime_product) >> 13) ^ working_mod);
    h = ((h & prime_mixed) << 4) | (((h ^ prime_product) / working_mod) >> 4);
    initial_hash = h;
    for (size_t j = 0; j < NUM_SBOXES; j++) {
        // Fill up sbox j
        for (size_t i = 0; i < SBOX_SIZE; i++) {
            // Mix using AND/OR operations with primes
            h = (h * prime_product) ^ prime_mod;
            sboxes[j][i] = ~((i ^ ((h & prime_low) ^ (h | prime_high))) ^ (h / prime_mod)) & 0xFF;
        }
    }
}

/**
 * @brief Destructor - cleans up heap-allocated S-boxes
 */
GoldenHash::~GoldenHash() {
    for (size_t i = 0; i < NUM_SBOXES; i++) {
        delete[] sboxes[i];
    }
}

/**
 * @brief Hash function implementation
 * @param data Pointer to data to hash
 * @param len Length of data in bytes
 * @return Hash value in range [0, N)
 */
uint64_t GoldenHash::hash(const uint8_t* data, size_t len) const {
    uint64_t h = initial_hash;
    uint64_t state = seed_ ^ prime_product;
    size_t i = 0;
    // Process 8 bytes at a time
    const uint64_t* data64 = reinterpret_cast<const uint64_t*>(data);
    size_t len64 = len / 8;
    uint64_t sbox_index = ~state;
    for (size_t j = 0; j < len64; j++) {
        uint64_t chunk = data64[j];
        // Mix the 64-bit chunk into state
        state ^= chunk;
        state *= prime_low;
        state ^= (state >> 17);
        // Process all 8 bytes in parallel using S-boxes
        uint64_t mixed = state ^ h;
        // Extract 5 x 12-bit indices from the 64-bit mixed value
        uint8_t c1 = sboxes[++sbox_index & 7][mixed & 0xFFF];
        uint8_t c2 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
        uint8_t c3 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
        uint8_t c4 = sboxes[++sbox_index & 7][(mixed >> 36) & 0xFFF];
        uint8_t c5 = sboxes[++sbox_index & 7][(mixed >> 48) & 0xFFF];
        // For remaining indices, mix state differently
        mixed = (state << 13) ^ (h >> 7);
        uint8_t c6 = sboxes[++sbox_index & 7][mixed & 0xFFF];
        uint8_t c7 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
        uint8_t c8 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
        // Combine all compressed values
        uint64_t compressed = ((uint64_t)c1 << 56) | ((uint64_t)c2 << 48) | 
                             ((uint64_t)c3 << 40) | ((uint64_t)c4 << 32) |
                             ((uint64_t)c5 << 24) | ((uint64_t)c6 << 16) | 
                             ((uint64_t)c7 << 8) | c8;
        h ^= compressed;
        h *= prime_high;
        h ^= (h >> 29);
        i += 8;
    }
    // Process remaining bytes
    for (; i < len; i++) {
        // Mix input byte into state
        state = (state << 8) | data[i];
        state *= prime_low;
        state ^= (state >> 17);
        // Use 3 S-box compressions per byte for irreversibility
        uint8_t compressed1 = sboxes[++sbox_index & 7][(state ^ h) & 0xFFF];
        uint8_t compressed2 = sboxes[++sbox_index & 7][((state >> 12) ^ (h >> 6)) & 0xFFF];
        uint8_t compressed3 = sboxes[++sbox_index & 7][((state >> 24) ^ (h >> 18)) & 0xFFF];
        h = (h << 24) | (compressed1 << 16) | (compressed2 << 8) | compressed3;
        h ^= state;
        h *= prime_high;
        h ^= (h >> 29);
    }
    // Final avalanche
    h ^= len * prime_mixed;
    h ^= (h >> 33);
    h *= prime_low;
    h ^= (h >> 27);
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
 * @brief Get the factorization of the working modulus
 * @return Vector of factors
 */
const std::vector<uint64_t>& GoldenHash::get_factors() const { return factors; }

/**
 * @brief Analyze S-box distribution and properties
 */
void GoldenHash::analyze_sboxes() const {
    // ANSI color codes
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string BOLD = "\033[1m";
    
    std::cout << "\n" << BOLD << "=== S-BOX ANALYSIS (12-bit → 8-bit compression) ===" << RESET << "\n";
    
    // Create header for table
    std::cout << std::left << std::setw(10) << "S-box"
              << std::setw(12) << "Unused"
              << std::setw(12) << "Std Dev"
              << std::setw(15) << "Bit Changes"
              << std::setw(18) << "Diff Unif"
              << std::setw(15) << "Linearity"
              << std::setw(15) << "Sequential"
              << "\n";
    std::cout << std::string(95, '-') << "\n";
    
    for (size_t j = 0; j < NUM_SBOXES; j++) {
        // Count frequency of each output value
        std::vector<int> output_freq(256, 0);
        for (size_t i = 0; i < SBOX_SIZE; i++) {
            output_freq[sboxes[j][i]]++;
        }
        
        // Find min/max frequencies
        int min_freq = SBOX_SIZE, max_freq = 0;
        int unused_outputs = 0;
        for (int i = 0; i < 256; i++) {
            if (output_freq[i] == 0) unused_outputs++;
            if (output_freq[i] > 0 && output_freq[i] < min_freq) min_freq = output_freq[i];
            if (output_freq[i] > max_freq) max_freq = output_freq[i];
        }
        
        // Calculate average frequency and standard deviation
        double avg_freq = double(SBOX_SIZE) / 256.0;
        double variance = 0;
        for (int i = 0; i < 256; i++) {
            double diff = output_freq[i] - avg_freq;
            variance += diff * diff;
        }
        double std_dev = std::sqrt(variance / 256);
        
        // Bit distribution analysis
        std::vector<int> bit_count(8, 0);
        for (size_t i = 0; i < SBOX_SIZE; i++) {
            uint8_t val = sboxes[j][i];
            for (int bit = 0; bit < 8; bit++) {
                if (val & (1 << bit)) bit_count[bit]++;
            }
        }
        
        
        // Check for obvious patterns
        int sequential_count = 0;
        for (size_t i = 1; i < SBOX_SIZE; i++) {
            if (sboxes[j][i] == (sboxes[j][i-1] + 1) % 256) sequential_count++;
        }
        
        // 1. Avalanche test: How many output bits change when input changes by 1
        double total_bit_changes = 0;
        int avalanche_tests = 0;
        for (size_t i = 0; i < SBOX_SIZE - 1; i++) {
            uint8_t val1 = sboxes[j][i];
            uint8_t val2 = sboxes[j][i + 1];
            uint8_t diff = val1 ^ val2;
            int bits_changed = __builtin_popcount(diff);
            total_bit_changes += bits_changed;
            avalanche_tests++;
        }
        double avg_avalanche = total_bit_changes / avalanche_tests;
        
        // 2. Differential uniformity: max occurrences of same output difference
        // For a specific input difference, count how many times each output difference occurs
        // We'll test a few specific input differences
        int max_diff_count = 0;
        std::vector<int> test_diffs = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
        
        for (int in_diff : test_diffs) {
            std::map<int, int> out_diff_count;
            for (size_t i = 0; i < SBOX_SIZE - in_diff; i++) {
                int out_diff = (sboxes[j][i + in_diff] - sboxes[j][i] + 256) & 0xFF;
                out_diff_count[out_diff]++;
            }
            for (const auto& pair : out_diff_count) {
                if (pair.second > max_diff_count) max_diff_count = pair.second;
            }
        }
        
        // 3. Non-linearity measure: distance from nearest affine function
        int min_matches = SBOX_SIZE;
        for (int a = 0; a < 8; a++) {  // Test a few linear functions
            for (int b = 0; b < 8; b++) {
                int matches = 0;
                for (size_t i = 0; i < SBOX_SIZE; i++) {
                    uint8_t expected = (a * i + b) & 0xFF;
                    if (sboxes[j][i] == expected) matches++;
                }
                if (matches < min_matches) min_matches = matches;
            }
        }
        
        // Output row in table
        std::cout << std::left << std::setw(10) << ("S-box " + std::to_string(j));
        
        // Unused outputs (red if > 0)
        if (unused_outputs > 0) {
            std::cout << RED << std::setw(12) << unused_outputs << RESET;
        } else {
            std::cout << GREEN << std::setw(12) << unused_outputs << RESET;
        }
        
        // Standard deviation
        std::cout << std::fixed << std::setprecision(2) << std::setw(12) << std_dev;
        
        // Bit changes (green if close to 4.0)
        if (std::abs(avg_avalanche - 4.0) < 0.5) {
            std::cout << GREEN << std::setw(15) << avg_avalanche << RESET;
        } else if (std::abs(avg_avalanche - 4.0) < 1.0) {
            std::cout << YELLOW << std::setw(15) << avg_avalanche << RESET;
        } else {
            std::cout << RED << std::setw(15) << avg_avalanche << RESET;
        }
        
        // Differential uniformity (color based on quality)
        if (max_diff_count <= 64) {
            std::cout << GREEN << std::setw(18) << max_diff_count << RESET;
        } else if (max_diff_count <= 128) {
            std::cout << YELLOW << std::setw(18) << max_diff_count << RESET;
        } else {
            std::cout << RED << std::setw(18) << max_diff_count << RESET;
        }
        
        // Linearity
        std::cout << std::setw(15) << (std::to_string(min_matches) + "/" + std::to_string(SBOX_SIZE));
        
        // Sequential patterns
        if (sequential_count < 100) {
            std::cout << GREEN << std::setw(15) << sequential_count << RESET;
        } else {
            std::cout << RED << std::setw(15) << sequential_count << RESET;
        }
        
        std::cout << "\n";
    }
    
    // Print legend
    std::cout << "\n" << BOLD << "Legend:" << RESET << "\n";
    std::cout << "  Unused: " << GREEN << "0 is good" << RESET << ", " << RED << ">0 is bad" << RESET << "\n";
    std::cout << "  Bit Changes: " << GREEN << "~4.0 is ideal" << RESET << " (50% avalanche)\n";
    std::cout << "  Diff Uniformity: " << GREEN << "≤64 good" << RESET << ", " << YELLOW << "≤128 okay" << RESET << ", " << RED << ">128 poor" << RESET << " (for 12→8 bit S-box)\n";
    std::cout << "  Lower is better for: Linearity, Sequential\n";
}

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