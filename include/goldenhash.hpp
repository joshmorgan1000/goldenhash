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
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>
    #define HAS_AVX2 __AVX2__
#elif defined(__aarch64__) || defined(_M_ARM64)
    #include <arm_neon.h>
    #define HAS_NEON 1
#endif

namespace goldenhash {

static const long double GOLDEN_RATIO = (1 + std::sqrt(5)) / 2;


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

struct CollectiveMetrics {
    uint64_t unique_hashes;
    uint64_t total_collisions;
    double distribution_uniformity;
    double expected_collisions;
    double collision_ratio;
    uint64_t max_bucket_load;
    double avalanche_score;
    double chi_square;
    uint64_t table_size;
    uint64_t prime_high;
    uint64_t prime_low;
    uint64_t working_modulus;
    double performance_ns_per_hash;
    std::vector<uint64_t> factors{};
    bool is_better_than(const CollectiveMetrics& other) const {
        double score = 0;
        score -= ((0.5 - avalanche_score) * (0.5 - avalanche_score)) - ((0.5 - other.avalanche_score) * (0.5 - other.avalanche_score));
        score -= ((1.0 - chi_square) * (1.0 - chi_square)) - ((1.0 - other.chi_square) * (1.0 - other.chi_square));
        return score > 0;
    }
    std::string to_json() {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"unique_hashes\": " << unique_hashes << ",\n";
        ss << "  \"total_collisions\": " << total_collisions << ",\n";
        ss << "  \"distribution_uniformity\": " << distribution_uniformity << ",\n";
        ss << "  \"expected_collisions\": " << expected_collisions << ",\n";
        ss << "  \"collision_ratio\": " << collision_ratio << ",\n";
        ss << "  \"max_bucket_load\": " << max_bucket_load << ",\n";
        ss << "  \"avalanche_score\": " << avalanche_score << ",\n";
        ss << "  \"chi_square\": " << chi_square << ",\n";
        ss << "  \"table_size\": " << table_size << ",\n";
        ss << "  \"prime_high\": " << prime_high << ",\n";
        ss << "  \"prime_low\": " << prime_low << ",\n";
        ss << "  \"working_modulus\": " << working_modulus << ",\n";
        ss << "  \"performance_ns_per_hash\": " << performance_ns_per_hash << ",\n";
        ss << "  \"factors\": [";
        for (size_t i = 0; i < factors.size(); i++) {
            ss << factors[i];
            if (i < factors.size() - 1) ss << ", ";
        }
        ss << "]\n";
        ss << "}\n";
        return ss.str();
    }
    std::string to_summary() {
        std::stringstream ss;
        ss << "Unique: " << unique_hashes
           << " Collisions: " << total_collisions
           << " Expected: " << expected_collisions
           << " Distribution: " << distribution_uniformity
           << " Ratio: " << collision_ratio
           << " Max load: " << max_bucket_load
           << " Avalanche: " << avalanche_score
           << " Chi^2: " << chi_square
           << " Size: " << table_size
           << " High: " << prime_high
           << " Low: " << prime_low
           << " Mod: " << working_modulus
           << " ns/hash: " 
           << std::fixed 
           << std::setprecision(2) 
           << performance_ns_per_hash 
           << "\n";
        return ss.str();
    }
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
     * @brief Destructor - cleans up heap-allocated S-boxes
     */
    ~GoldenHash();
    
    /**
     * @brief Hash function
     * @param data Pointer to data to hash
     * @param len Length of data in bytes
     * @return Hash value in range [0, N)
     */
    inline uint64_t hash(const uint8_t* data, size_t len) const {
        uint64_t h = initial_hash;
        uint64_t state = seed_ ^ prime_product;
        size_t i = 0;
        
#ifdef HAS_AVX2
        // Process 32 bytes at a time with AVX2
        if (len >= 32) {
            const uint64_t* data64 = reinterpret_cast<const uint64_t*>(data);
            size_t chunks32 = len / 32;
            uint64_t sbox_index = ~state;
            
            for (size_t chunk = 0; chunk < chunks32; chunk++) {
                // Prefetch next chunk's data and S-box entries
                if (chunk + 1 < chunks32) {
                    __builtin_prefetch(&data64[(chunk + 1) * 4], 0, 3);
                    __builtin_prefetch(&sboxes[(sbox_index + 8) & 7][0], 0, 3);
                }
                
                // Load 4x64-bit values
                uint64_t v0 = data64[chunk * 4 + 0];
                uint64_t v1 = data64[chunk * 4 + 1];
                uint64_t v2 = data64[chunk * 4 + 2];
                uint64_t v3 = data64[chunk * 4 + 3];
                
                // Process first 8 bytes
                state ^= v0;
                state *= prime_low;
                state ^= (state >> 17);
                
                uint64_t mixed = state ^ h;
                uint8_t c1 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                uint8_t c2 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                uint8_t c3 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                uint8_t c4 = sboxes[++sbox_index & 7][(mixed >> 36) & 0xFFF];
                uint8_t c5 = sboxes[++sbox_index & 7][(mixed >> 48) & 0xFFF];
                
                mixed = (state << 13) ^ (h >> 7);
                uint8_t c6 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                uint8_t c7 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                uint8_t c8 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                
                uint64_t compressed1 = ((uint64_t)c1 << 56) | ((uint64_t)c2 << 48) | 
                                      ((uint64_t)c3 << 40) | ((uint64_t)c4 << 32) |
                                      ((uint64_t)c5 << 24) | ((uint64_t)c6 << 16) | 
                                      ((uint64_t)c7 << 8) | c8;
                
                h ^= compressed1;
                h *= prime_high;
                h ^= (h >> 29);
                
                // Process second 8 bytes
                state ^= v1;
                state *= prime_low;
                state ^= (state >> 17);
                
                mixed = state ^ h;
                c1 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c2 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c3 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                c4 = sboxes[++sbox_index & 7][(mixed >> 36) & 0xFFF];
                c5 = sboxes[++sbox_index & 7][(mixed >> 48) & 0xFFF];
                
                mixed = (state << 13) ^ (h >> 7);
                c6 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c7 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c8 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                
                uint64_t compressed2 = ((uint64_t)c1 << 56) | ((uint64_t)c2 << 48) | 
                                      ((uint64_t)c3 << 40) | ((uint64_t)c4 << 32) |
                                      ((uint64_t)c5 << 24) | ((uint64_t)c6 << 16) | 
                                      ((uint64_t)c7 << 8) | c8;
                
                h ^= compressed2;
                h *= prime_high;
                h ^= (h >> 29);
                
                // Process third and fourth 8 bytes similarly
                state ^= v2;
                state *= prime_low;
                state ^= (state >> 17);
                
                mixed = state ^ h;
                c1 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c2 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c3 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                c4 = sboxes[++sbox_index & 7][(mixed >> 36) & 0xFFF];
                c5 = sboxes[++sbox_index & 7][(mixed >> 48) & 0xFFF];
                
                mixed = (state << 13) ^ (h >> 7);
                c6 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c7 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c8 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                
                uint64_t compressed3 = ((uint64_t)c1 << 56) | ((uint64_t)c2 << 48) | 
                                      ((uint64_t)c3 << 40) | ((uint64_t)c4 << 32) |
                                      ((uint64_t)c5 << 24) | ((uint64_t)c6 << 16) | 
                                      ((uint64_t)c7 << 8) | c8;
                
                h ^= compressed3;
                h *= prime_high;
                h ^= (h >> 29);
                
                // Process fourth 8 bytes
                state ^= v3;
                state *= prime_low;
                state ^= (state >> 17);
                
                mixed = state ^ h;
                c1 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c2 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c3 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                c4 = sboxes[++sbox_index & 7][(mixed >> 36) & 0xFFF];
                c5 = sboxes[++sbox_index & 7][(mixed >> 48) & 0xFFF];
                
                mixed = (state << 13) ^ (h >> 7);
                c6 = sboxes[++sbox_index & 7][mixed & 0xFFF];
                c7 = sboxes[++sbox_index & 7][(mixed >> 12) & 0xFFF];
                c8 = sboxes[++sbox_index & 7][(mixed >> 24) & 0xFFF];
                
                uint64_t compressed4 = ((uint64_t)c1 << 56) | ((uint64_t)c2 << 48) | 
                                      ((uint64_t)c3 << 40) | ((uint64_t)c4 << 32) |
                                      ((uint64_t)c5 << 24) | ((uint64_t)c6 << 16) | 
                                      ((uint64_t)c7 << 8) | c8;
                
                h ^= compressed4;
                h *= prime_high;
                h ^= (h >> 29);
                
                i += 32;
            }
            
            // Update pointers for remaining data
            data += i;
            len -= i;
            i = 0;
        }
#endif
        
        // Process 8 bytes at a time (original code with prefetching)
        const uint64_t* data64 = reinterpret_cast<const uint64_t*>(data);
        size_t len64 = len / 8;
        uint64_t sbox_index = ~state;
        
        for (size_t j = 0; j < len64; j++) {
            // Prefetch next S-box entries
            if (j + 1 < len64) {
                __builtin_prefetch(&sboxes[(sbox_index + 1) & 7][0], 0, 1);
            }
            
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
    uint64_t get_prime_mod() const {
        return prime_mod;
    }

    uint64_t get_prime_product() const {
        return prime_product;
    }

    uint64_t get_working_mod() const {
        return working_mod;
    }

    uint64_t get_prime_mixed() const {
        return prime_mixed;
    }

    uint64_t get_initial_hash() const {
        return initial_hash;
    }
    
    /**
     * @brief Get the factorization of the working modulus
     * @return Vector of factors
     */
    const std::vector<uint64_t>& get_factors() const;
    
    /**
     * @brief Analyze S-box distribution and properties
     */
    void analyze_sboxes() const;

    /**
     * @brief Runs a round of tests for a single table size
     * @param table_size Size of the hash table to test
     * @param num_tests Number of tests to run
     * @return CollectiveMetrics containing the results of the tests
     */
    static CollectiveMetrics run_tests_for(uint64_t table_size, uint64_t num_tests) {
        GoldenHash hasher(table_size);
        // Generate test data
        std::mt19937_64 rng(42);
        std::vector<std::vector<uint8_t>> test_data;
        for (size_t i = 0; i < num_tests; i++) {
            std::vector<uint8_t> data(16 + (i % 48)); // Vary size 16-64 bytes
            for (auto& byte : data) {
                byte = rng() & 0xFF;
            }
            test_data.push_back(data);
        }
        // Hash and collect statistics, including avalanche
        std::vector<uint64_t> hash_counts(table_size, 0);
        size_t total_bit_changes = 0;
        size_t total_bit_tests = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < test_data.size(); i++) {
            const auto& data = test_data[i];
            uint64_t h = hasher.hash(data.data(), data.size());
            hash_counts[h]++;
            // Test avalanche for a subset of inputs (every 100th) to keep performance reasonable
            if (i % 100 == 0) {
                // Test single bit flips
                for (size_t byte_idx = 0; byte_idx < data.size() && byte_idx < 32; byte_idx++) {
                    for (int bit = 0; bit < 8; bit++) {
                        // Make a copy and flip bit
                        std::vector<uint8_t> modified = data;
                        modified[byte_idx] ^= (1 << bit);
                        // Hash modified
                        uint64_t h2 = hasher.hash(modified.data(), modified.size());
                        // Count differing bits (only bits that matter for table_size)
                        uint64_t diff = h ^ h2;
                        // For small table sizes, only count relevant bits
                        int bits_to_count = 64;
                        if (table_size < (1ULL << 32)) {
                            bits_to_count = 32 - __builtin_clz(table_size - 1) + 1;
                        }
                        uint64_t mask = (bits_to_count >= 64) ? ~0ULL : ((1ULL << bits_to_count) - 1);
                        total_bit_changes += __builtin_popcountll(diff & mask);
                        total_bit_tests++;
                    }
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate statistics
        uint64_t unique_hashes = 0;
        uint64_t max_collisions = 0;
        double expected = double(num_tests) / table_size;
        double chi_square = 0.0;
        
        for (uint64_t count : hash_counts) {
            if (count > 0) unique_hashes++;
            if (count > max_collisions) max_collisions = count;
            
            double diff = count - expected;
            chi_square += (diff * diff) / expected;
        }
        chi_square /= table_size;
        
        uint64_t total_collisions = num_tests - unique_hashes;
        
        // Calculate expected collisions (birthday paradox)
        double expected_unique = table_size * (1.0 - std::exp(-double(num_tests) / table_size));
        double expected_collisions = num_tests - expected_unique;
        double collision_ratio = (expected_collisions > 0) ? total_collisions / expected_collisions : 1.0;
        
        // Calculate avalanche score
        // For modular hashing, we need to account for the limited output space
        int output_bits = (table_size < 2) ? 1 : (64 - __builtin_clzll(table_size - 1));
        double avalanche_score = (total_bit_tests > 0) ? 
            double(total_bit_changes) / (total_bit_tests * output_bits) : 0.0;
        
        // Hash standard test vectors
        struct TestVector {
            const char* str;
            const char* name;
        };
        
        TestVector vectors[] = {
            {"", "empty"},
            {"a", "a"},
            {"abc", "abc"},
            {"message digest", "message_digest"},
            {"abcdefghijklmnopqrstuvwxyz", "alphabet"},
            {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "alphanumeric"},
            {"The quick brown fox jumps over the lazy dog", "fox"}
        };
        
        std::vector<std::pair<std::string, uint64_t>> test_hashes;
        for (const auto& vec : vectors) {
            uint64_t h = hasher.hash(reinterpret_cast<const uint8_t*>(vec.str), strlen(vec.str));
            test_hashes.push_back({vec.name, h});
        }

        CollectiveMetrics metrics;
        metrics.table_size = table_size;
        metrics.unique_hashes = unique_hashes;
        metrics.distribution_uniformity = std::sqrt(chi_square / table_size);
        metrics.total_collisions = total_collisions;
        metrics.expected_collisions = expected_collisions;
        metrics.collision_ratio = collision_ratio;
        metrics.avalanche_score = avalanche_score;
        metrics.chi_square = chi_square;
        metrics.prime_high = hasher.get_prime_high();
        metrics.prime_low = hasher.get_prime_low();
        metrics.working_modulus = hasher.get_working_mod();
        metrics.factors = hasher.get_factors();
        metrics.performance_ns_per_hash = (duration.count() * 1000.0 / num_tests);
        return metrics;
    }

    /**
     * @brief Runs a round of tests for a single table size
     * @param table_size Size of the hash table to test
     * @param num_tests Number of tests to run
     * @return The number of hashes per nanosecond for the given table size
     */
    static double speed_test(uint64_t table_size, uint64_t num_tests) {
        GoldenHash hasher(table_size);
        // Generate test data
        std::mt19937_64 rng(42);
        std::vector<std::vector<uint8_t>> test_data;
        for (size_t i = 0; i < num_tests; i++) {
            std::vector<uint8_t> data(16 + (i % 48)); // Vary size 16-64 bytes
            for (auto& byte : data) {
                byte = rng() & 0xFF;
            }
            test_data.push_back(data);
        }
        auto start = std::chrono::high_resolution_clock::now();
        std::stringstream ss;
        for (size_t i = 0; i < test_data.size(); i++) {
            const auto& data = test_data[i];
            uint64_t h = hasher.hash(data.data(), data.size());
            if ((i & 0xFFF) == 0) {
                ss << h << " ";
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        // Calculate how many ns per hash
        return double(num_tests) / (duration.count());
    }

    /**
     * @brief Find the hash table with the best metrics for a given target
     * 
     * This function searches for the best hash table size based on the target size,
     * compares avalanche effect, collision rate, chi-square distribution, and
     * number of collisions vs. expected collisions based on the birthday paradox.
     * 
     * @param target_size Target size for the hash table
     * @param multiple_of Multiple of which the table size should be a multiple of (default: 1)
     * @param interations_to_search Number of iterations to search for the best table size (default: 100)
     * @param seeds_to_check Number of different seeds to check for the best table size (default: 20)
     */
    static void find_best_table_size(int64_t target_size,
                                     int64_t sizes_to_check = 1000,
                                     int64_t multiple_of = 1, 
                                     int64_t interations_to_search = 20000) {
        // Find a range between target_size where the modulus of the target size is == multiple_of, between -500 and +500 of target_size
        CollectiveMetrics best_metrics;
        bool value_collectd = false;
        // Find a range between target_size where the modulus of the target size is == multiple_of, between -500 and +500 of target_size
        int64_t halfway = sizes_to_check / 2 * multiple_of;
        int64_t low_size = target_size - halfway;
        int64_t high_size = target_size + halfway;
        while (low_size % multiple_of != 0) {
            low_size++;
        }
        while (high_size % multiple_of != 0) {
            high_size++;
        }
        for (int64_t i = low_size; i <= high_size; i += multiple_of) {
            if ((i < 500) || i > std::numeric_limits<int64_t>::max() - 500) {
                std::cout << "Size is too small or too large for GoldenHash: " << i << "\n";
                return;
            }
            CollectiveMetrics new_metrics = run_tests_for(i, interations_to_search);
            if (!value_collectd || new_metrics.is_better_than(best_metrics)) {
                best_metrics = new_metrics;
                value_collectd = true;
                std::cout << "New best metrics found for size " << i << ":\n";
                std::cout << best_metrics.to_summary();
            }
        }
        if (!value_collectd) {
            std::cout << "No suitable table size found in the range.\n";
        } else {
            std::cout << "Best metrics found:\n";
            std::cout << best_metrics.to_json() << "\n"
                      << "Now running a full barage of tests on this final size.\n";
            double speed = speed_test(best_metrics.table_size, 1000000);
            std::cout << "Speed test for best size " << best_metrics.table_size << ": "
                      << std::fixed << std::setprecision(8)
                      << speed << " ns/hash\n";
            CollectiveMetrics final_metrics = run_tests_for(best_metrics.table_size, 1000000);
            std::cout << "Final metrics after full barrage:\n";
            std::cout << final_metrics.to_json() << "\n";
        }
    }

private:
    uint64_t N;              // Table size
    uint64_t prime_high;     // Prime near N/φ
    uint64_t prime_low;      // Prime near N/φ²
    uint64_t prime_product;
    uint64_t prime_mod;
    uint64_t working_mod;
    uint64_t prime_mixed;    // Mixed prime for compression
    uint64_t initial_hash;
    std::vector<uint64_t> factors;
    uint64_t seed_;          // Seed value
    
    // Compressive S-boxes for irreversibility
    static constexpr size_t SBOX_SIZE = (1 << 12);  // 12-bit to 8-bit compression (4KB per S-box)
    static constexpr size_t NUM_SBOXES = 8;
    
    // Direct array allocation for better performance and cache locality
    alignas(64) uint8_t* sboxes[NUM_SBOXES];

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
