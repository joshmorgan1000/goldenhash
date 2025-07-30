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
 * @file goldenhash_cipher.cpp
 * @brief Implementation of experimental stream cipher using GoldenHash
 */

#include "goldenhash_cipher.hpp"
#include <algorithm>
#include <random>
#include <bitset>
#include <iomanip>

namespace goldenhash {

// Static member initialization
std::vector<uint64_t> GoldenHashCipher::prime_table_sizes;
bool GoldenHashCipher::tables_initialized = false;

/**
 * @brief Find prime-dense regions for table sizes
 * We want 2048 values that maximize prime density in their neighborhoods
 */
void GoldenHashCipher::init_prime_tables() {
    if (tables_initialized) return;
    
    prime_table_sizes.reserve(2048);
    
    // Helper to check if a number is prime
    auto is_prime = [](uint64_t n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (uint64_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    };
    
    // Helper to count primes in a range
    auto count_primes_in_range = [&is_prime](uint64_t center, uint64_t radius) {
        uint64_t count = 0;
        uint64_t start = (center > radius) ? center - radius : 2;
        uint64_t end = center + radius;
        for (uint64_t i = start; i <= end; i++) {
            if (is_prime(i)) count++;
        }
        return count;
    };
    
    // Structure to hold candidates
    struct Candidate {
        uint64_t value;
        uint64_t prime_density;
    };
    
    std::vector<Candidate> candidates;
    
    // Search in multiple ranges for good distribution
    // We want sizes from ~1000 to ~100000 for practical use
    std::vector<std::pair<uint64_t, uint64_t>> ranges = {
        {1000, 5000},      // Small tables
        {5000, 20000},     // Medium tables
        {20000, 50000},    // Large tables
        {50000, 100000}    // Very large tables
    };
    
    for (const auto& [range_start, range_end] : ranges) {
        uint64_t step = (range_end - range_start) / 600; // Sample ~600 points per range
        for (uint64_t i = range_start; i < range_end; i += step) {
            // Look for primes near this value
            for (uint64_t delta = 0; delta < step && i + delta < range_end; delta++) {
                uint64_t val = i + delta;
                if (is_prime(val)) {
                    // Count primes in neighborhood
                    uint64_t density = count_primes_in_range(val, 50);
                    candidates.push_back({val, density});
                    break; // Found a prime, move to next step
                }
            }
        }
    }
    
    // Sort by prime density (descending)
    std::sort(candidates.begin(), candidates.end(), 
              [](const Candidate& a, const Candidate& b) {
                  return a.prime_density > b.prime_density;
              });
    
    // Take top 2048 candidates
    size_t count = std::min(size_t(2048), candidates.size());
    for (size_t i = 0; i < count; i++) {
        prime_table_sizes.push_back(candidates[i].value);
    }
    
    // Sort the final list for consistent indexing
    std::sort(prime_table_sizes.begin(), prime_table_sizes.end());
    
    // If we don't have enough, fill with additional primes
    if (prime_table_sizes.size() < 2048) {
        uint64_t start = prime_table_sizes.empty() ? 1009 : prime_table_sizes.back() + 2;
        while (prime_table_sizes.size() < 2048) {
            if (is_prime(start)) {
                prime_table_sizes.push_back(start);
            }
            start += 2; // Only check odd numbers
        }
    }
    
    tables_initialized = true;
}

/**
 * @brief Get prime table sizes
 */
const std::vector<uint64_t>& GoldenHashCipher::get_prime_table_sizes() {
    if (!tables_initialized) {
        init_prime_tables();
    }
    return prime_table_sizes;
}

/**
 * @brief Get table size for index
 */
uint64_t GoldenHashCipher::get_table_size(size_t index) {
    if (!tables_initialized) {
        init_prime_tables();
    }
    return prime_table_sizes[index & MAX_TABLE_INDEX];
}

/**
 * @brief Constructor
 */
GoldenHashCipher::GoldenHashCipher(const uint8_t key[KEY_SIZE]) : counter(0) {
    if (!tables_initialized) {
        init_prime_tables();
    }
    
    // Extract 4 16-bit subkeys
    for (size_t i = 0; i < NUM_CIPHERS; i++) {
        uint16_t subkey = (key[i*2] << 8) | key[i*2 + 1];
        
        // Extract table index (11 bits) and seed (5 bits)
        size_t table_index = (subkey >> SEED_BITS) & MAX_TABLE_INDEX;
        uint64_t seed = subkey & MAX_SEED;
        
        // Create hasher with selected table size and seed
        uint64_t table_size = prime_table_sizes[table_index];
        hashers[i] = std::make_unique<GoldenHash>(table_size, seed);
    }
}

/**
 * @brief Process data (encrypt/decrypt)
 */
void GoldenHashCipher::process(const uint8_t* data, size_t len, uint8_t* output) {
    for (size_t i = 0; i < len; i++) {
        // Create keystream byte by chaining hashers
        uint64_t stream_value = counter + i;
        
        // Chain through all 4 hashers
        for (size_t j = 0; j < NUM_CIPHERS; j++) {
            // Hash the current stream value
            uint8_t input[8];
            for (size_t k = 0; k < 8; k++) {
                input[k] = (stream_value >> (k * 8)) & 0xFF;
            }
            
            stream_value = hashers[j]->hash(input, sizeof(input));
            
            // Mix with previous result for better diffusion
            if (j > 0) {
                stream_value ^= (stream_value >> 32);
                stream_value *= 0xff51afd7ed558ccdULL;
                stream_value ^= (stream_value >> 32);
            }
        }
        
        // Extract keystream byte and XOR with data
        output[i] = data[i] ^ (stream_value & 0xFF);
    }
    
    counter += len;
}

/**
 * @brief Analyze key properties
 */
void GoldenHashCipher::analyze_key(const uint8_t key[KEY_SIZE]) {
    if (!tables_initialized) {
        init_prime_tables();
    }
    
    // Helper to check if a number is prime
    auto is_prime = [](uint64_t n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (uint64_t i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    };
    
    std::cout << "Key Analysis:\n";
    std::cout << "=============\n";
    
    for (size_t i = 0; i < NUM_CIPHERS; i++) {
        uint16_t subkey = (key[i*2] << 8) | key[i*2 + 1];
        size_t table_index = (subkey >> SEED_BITS) & MAX_TABLE_INDEX;
        uint64_t seed = subkey & MAX_SEED;
        uint64_t table_size = prime_table_sizes[table_index];
        
        std::cout << "Cipher " << i << ":\n";
        std::cout << "  16-bit subkey: 0x" << std::hex << subkey << std::dec << "\n";
        std::cout << "  Table index: " << table_index << "\n";
        std::cout << "  Table size: " << table_size << (is_prime(table_size) ? " (prime)" : " (composite)") << "\n";
        std::cout << "  Seed: " << seed << "\n";
        std::cout << "  Binary: " << std::bitset<16>(subkey) << "\n\n";
    }
}


/**
 * @brief Test avalanche effect
 */
void CipherAnalyzer::test_avalanche(size_t num_tests) {
    std::cout << "\nAvalanche Effect Test\n";
    std::cout << "====================\n";
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    double total_bit_changes = 0;
    size_t total_comparisons = 0;
    
    for (size_t test = 0; test < num_tests; test++) {
        // Generate random key
        uint8_t key1[8], key2[8];
        for (size_t i = 0; i < 8; i++) {
            key1[i] = dist(gen);
            key2[i] = key1[i];
        }
        
        // Flip one random bit
        size_t byte_pos = test % 8;
        size_t bit_pos = (test / 8) % 8;
        key2[byte_pos] ^= (1 << bit_pos);
        
        // Create ciphers
        GoldenHashCipher cipher1(key1);
        GoldenHashCipher cipher2(key2);
        
        // Generate and compare output
        uint8_t input[64] = {0};
        uint8_t output1[64], output2[64];
        
        cipher1.process(input, 64, output1);
        cipher2.process(input, 64, output2);
        
        // Count bit differences
        size_t bit_diffs = 0;
        for (size_t i = 0; i < 64; i++) {
            uint8_t diff = output1[i] ^ output2[i];
            bit_diffs += __builtin_popcount(diff);
        }
        
        total_bit_changes += bit_diffs;
        total_comparisons++;
    }
    
    double avg_bit_changes = total_bit_changes / total_comparisons;
    double expected = 64 * 8 * 0.5; // 50% of bits should change
    
    std::cout << "Average bit changes: " << avg_bit_changes << " / 512\n";
    std::cout << "Expected (50%): " << expected << "\n";
    std::cout << "Avalanche ratio: " << (avg_bit_changes / 512.0) << "\n";
}

/**
 * @brief Test diffusion
 */
void CipherAnalyzer::test_diffusion(size_t num_tests) {
    std::cout << "\nDiffusion Test\n";
    std::cout << "==============\n";
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    // Test how input changes affect output
    uint8_t key[8];
    for (size_t i = 0; i < 8; i++) {
        key[i] = dist(gen);
    }
    
    GoldenHashCipher cipher(key);
    
    double total_bit_changes = 0;
    
    for (size_t test = 0; test < num_tests; test++) {
        uint8_t input1[64], input2[64];
        for (size_t i = 0; i < 64; i++) {
            input1[i] = dist(gen);
            input2[i] = input1[i];
        }
        
        // Change one bit
        size_t pos = test % 64;
        input2[pos] ^= 1;
        
        uint8_t output1[64], output2[64];
        cipher.process(input1, 64, output1);
        
        // Reset cipher state for consistent comparison
        GoldenHashCipher cipher2(key);
        cipher2.process(input2, 64, output2);
        
        // Count differences
        size_t bit_diffs = 0;
        for (size_t i = 0; i < 64; i++) {
            uint8_t diff = output1[i] ^ output2[i];
            bit_diffs += __builtin_popcount(diff);
        }
        
        total_bit_changes += bit_diffs;
    }
    
    double avg_changes = total_bit_changes / num_tests;
    std::cout << "Average output bit changes per input bit flip: " << avg_changes << " / 512\n";
    std::cout << "Diffusion ratio: " << (avg_changes / 512.0) << "\n";
}

/**
 * @brief Test keystream patterns
 */
void CipherAnalyzer::test_keystream_patterns(const uint8_t key[8], size_t stream_length) {
    std::cout << "\nKeystream Pattern Analysis\n";
    std::cout << "=========================\n";
    
    GoldenHashCipher cipher(key);
    
    // Generate keystream
    std::vector<uint8_t> zeros(stream_length, 0);
    std::vector<uint8_t> keystream(stream_length);
    cipher.process(zeros.data(), stream_length, keystream.data());
    
    // Analyze byte frequency
    std::array<size_t, 256> byte_freq = {0};
    for (uint8_t byte : keystream) {
        byte_freq[byte]++;
    }
    
    // Calculate chi-square for uniform distribution
    double expected = stream_length / 256.0;
    double chi_square = 0;
    for (size_t freq : byte_freq) {
        double diff = freq - expected;
        chi_square += (diff * diff) / expected;
    }
    
    std::cout << "Stream length: " << stream_length << "\n";
    std::cout << "Chi-square statistic: " << chi_square << "\n";
    std::cout << "Degrees of freedom: 255\n";
    std::cout << "Expected range (p=0.05): [208.1, 304.9]\n";
    
    // Check for obvious patterns
    size_t repeated_bytes = 0;
    for (size_t i = 1; i < stream_length; i++) {
        if (keystream[i] == keystream[i-1]) repeated_bytes++;
    }
    
    std::cout << "Consecutive repeated bytes: " << repeated_bytes 
              << " (" << (100.0 * repeated_bytes / stream_length) << "%)\n";
    std::cout << "Expected: ~" << (stream_length / 256.0) 
              << " (" << (100.0 / 256.0) << "%)\n";
}

/**
 * @brief Test correlation between keys
 */
void CipherAnalyzer::test_key_correlation(size_t num_keys) {
    std::cout << "\nKey Correlation Test\n";
    std::cout << "===================\n";
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    // Generate random keys and their outputs
    std::vector<std::array<uint8_t, 8>> keys(num_keys);
    std::vector<std::array<uint8_t, 64>> outputs(num_keys);
    
    uint8_t input[64] = {0}; // Same input for all
    
    for (size_t i = 0; i < num_keys; i++) {
        for (size_t j = 0; j < 8; j++) {
            keys[i][j] = dist(gen);
        }
        
        GoldenHashCipher cipher(keys[i].data());
        cipher.process(input, 64, outputs[i].data());
    }
    
    // Calculate average Hamming distance between outputs
    double total_distance = 0;
    size_t comparisons = 0;
    
    for (size_t i = 0; i < num_keys; i++) {
        for (size_t j = i + 1; j < num_keys; j++) {
            size_t distance = 0;
            for (size_t k = 0; k < 64; k++) {
                uint8_t diff = outputs[i][k] ^ outputs[j][k];
                distance += __builtin_popcount(diff);
            }
            total_distance += distance;
            comparisons++;
        }
    }
    
    double avg_distance = total_distance / comparisons;
    double expected = 64 * 8 * 0.5; // 50% bits different
    
    std::cout << "Keys tested: " << num_keys << "\n";
    std::cout << "Average Hamming distance: " << avg_distance << " / 512\n";
    std::cout << "Expected (independent): " << expected << "\n";
    std::cout << "Independence ratio: " << (avg_distance / 512.0) << "\n";
}

/**
 * @brief Run all tests
 */
void CipherAnalyzer::run_all_tests() {
    std::cout << "GoldenHash Cipher Analysis\n";
    std::cout << "==========================\n";
    
    // Show prime table info
    const auto& primes = GoldenHashCipher::get_prime_table_sizes();
    std::cout << "Prime table sizes: " << primes.size() << "\n";
    std::cout << "Range: [" << primes.front() << ", " << primes.back() << "]\n";
    
    // Example key analysis
    uint8_t example_key[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    GoldenHashCipher::analyze_key(example_key);
    
    // Run tests
    test_avalanche(10000);
    test_diffusion(10000);
    test_keystream_patterns(example_key, 1048576);
    test_key_correlation(100);
}

} // namespace goldenhash