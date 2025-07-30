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
 * @file main.cpp
 * @brief Main test program for GoldenHash library
 * @author Josh Morgan
 * @date 2025
 * 
 * This program runs comprehensive tests on the GoldenHash implementations,
 * including FloatHash and metrics analysis.
 */

#include <goldenhash.hpp>
#include <iostream>
#include <iomanip>
#include <vector>
#include <fstream>

using namespace goldenhash;

/**
 * @brief Main function - run all tests
 */
int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << " <table_size> <num_tests> [--json]\n";
        std::cerr << "Example: " << argv[0] << " 5829235 24000\n";
        std::cerr << "Example: " << argv[0] << " 5829235 24000 --json\n";
        return 1;
    }
    uint64_t table_size = std::stoull(argv[1]);
    size_t num_tests = std::stoull(argv[2]);
    bool json_output = (argc == 4 && std::string(argv[3]) == "--json");
    // Create hash function
    GoldenHash hasher(table_size);
    if (!json_output) {
        std::cout << "Modular Golden Ratio Hash Test\n";
        std::cout << "==============================\n\n";
        hasher.print_info();
    }
    
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
    
    if (!json_output) {
        std::cout << "\nRunning " << num_tests << " hash operations...\n";
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
    
    if (!json_output) {
        // Output results
        std::cout << "\nResults:\n";
        std::cout << "--------\n";
        std::cout << "Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "Performance: " << duration.count() * 1000.0 / num_tests << " ns/hash\n";
        std::cout << "Unique hashes: " << unique_hashes << "/" << num_tests << "\n";
        std::cout << "Total collisions: " << total_collisions << "\n";
        std::cout << "Expected collisions: " << expected_collisions << "\n";
        std::cout << "Collision ratio: " << collision_ratio << " (ideal: 1.0)\n";
        std::cout << "Max bucket load: " << max_collisions << "\n";
        std::cout << "Chi-square: " << chi_square << " (ideal: 1.0)\n";
        std::cout << "Avalanche score: " << avalanche_score << " (ideal: 0.5)\n";
        
        // Show test vectors
        std::cout << "\nTest vectors:\n";
        for (const auto& [name, hash] : test_hashes) {
            std::cout << "  H(\"" << name << "\"): " << hash << "\n";
        }
        
        // Predecessor function check (N/φ)
        std::cout << "\nPredecessor function check:\n";
        std::cout << "N / φ = " << table_size << " / " << GOLDEN_RATIO << " = " << table_size / GOLDEN_RATIO << "\n";
        std::cout << "φ * " << uint64_t(table_size / GOLDEN_RATIO) << " = " << GOLDEN_RATIO * uint64_t(table_size / GOLDEN_RATIO) << "\n";
        std::cout << "Difference from N: " << int64_t(table_size) - int64_t(GOLDEN_RATIO * uint64_t(table_size / GOLDEN_RATIO)) << "\n";
    } else {
        // JSON output
        std::cout << "{\n";
        std::cout << "  \"table_size\": " << table_size << ",\n";
        std::cout << "  \"unique_hashes\": " << unique_hashes << ",\n";
        std::cout << "  \"distribution_uniformity\": " << std::sqrt(chi_square / table_size) << ",\n";
        std::cout << "  \"total_collisions\": " << total_collisions << ",\n";
        std::cout << "  \"expected_collisions\": " << expected_collisions << ",\n";
        std::cout << "  \"collision_ratio\": " << collision_ratio << ",\n";
        std::cout << "  \"max_bucket_load\": " << max_collisions << ",\n";
        std::cout << "  \"avalanche_score\": " << avalanche_score << ",\n";
        std::cout << "  \"chi_square\": " << chi_square << ",\n";
        std::cout << "  \"prime_high\": " << hasher.get_prime_high() << ",\n";
        std::cout << "  \"prime_low\": " << hasher.get_prime_low() << ",\n";
        std::cout << "  \"working_modulus\": " << hasher.get_working_mod() << ",\n";
        std::cout << "  \"test_vectors\": {\n";
        for (size_t i = 0; i < test_hashes.size(); i++) {
            std::cout << "    \"" << test_hashes[i].first << "\": " << test_hashes[i].second;
            if (i < test_hashes.size() - 1) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  },\n";
        std::cout << "  \"factors\": \"";
        for (size_t i = 0; i < hasher.get_factors().size(); i++) {
            std::cout << hasher.get_factors()[i];
            if (i < hasher.get_factors().size() - 1) std::cout << ", ";
        }
        std::cout << "\",\n";
        std::cout << "  \"performance_ns_per_hash\": " 
                << (duration.count() * 1000.0 / num_tests) << "\n";
        std::cout << "}\n";
    }
    return 0;
}