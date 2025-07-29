/**
 * @file test_crocs_comprehensive.cpp
 * @brief Comprehensive testing and data collection for CROCS hash functions
 */

#include "../include/crocs.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <bitset>

using namespace crocs;

/**
 * @brief Data collection structure for analysis
 */
struct TestResults {
    struct TableSizeResults {
        uint64_t table_size;
        uint64_t golden_prime;
        double chi_square;
        double collision_ratio;  // actual/expected
        double avalanche_score;
        double distribution_uniformity;
        double ns_per_hash;
        uint64_t unique_hashes;
        uint64_t total_collisions;
        uint64_t max_bucket_load;
    };
    
    std::vector<TableSizeResults> results;
    
    void save_csv(const std::string& filename) const {
        std::ofstream file(filename);
        file << "table_size,golden_prime,chi_square,collision_ratio,avalanche_score,"
             << "distribution_uniformity,ns_per_hash,unique_hashes,total_collisions,"
             << "max_bucket_load,bits_needed,prime_distance_from_golden\n";
        
        for (const auto& r : results) {
            int bits_needed = 64 - __builtin_clzll(r.table_size - 1);
            uint64_t golden_value = static_cast<uint64_t>(r.table_size / PHI);
            int64_t prime_distance = static_cast<int64_t>(r.golden_prime) - 
                                    static_cast<int64_t>(golden_value);
            
            file << r.table_size << ","
                 << r.golden_prime << ","
                 << r.chi_square << ","
                 << r.collision_ratio << ","
                 << r.avalanche_score << ","
                 << r.distribution_uniformity << ","
                 << r.ns_per_hash << ","
                 << r.unique_hashes << ","
                 << r.total_collisions << ","
                 << r.max_bucket_load << ","
                 << bits_needed << ","
                 << prime_distance << "\n";
        }
        file.close();
    }
};

/**
 * @brief Comprehensive CROCS tester
 */
class CrocsTester {
private:
    static constexpr size_t DEFAULT_SAMPLES = 100000;
    std::mt19937 rng_{42};
    
public:
    /**
     * @brief Run avalanche test
     */
    double test_avalanche(uint64_t table_size) {
        CrocsHash32 hasher(table_size);
        const size_t num_tests = 1000;
        const size_t data_size = 16;
        std::vector<double> bit_flip_ratios;
        
        for (size_t i = 0; i < num_tests; ++i) {
            // Generate random data
            std::vector<uint8_t> data(data_size);
            for (auto& byte : data) {
                byte = rng_() & 0xFF;
            }
            
            uint32_t original = hasher.hash_bytes(data.data(), data.size());
            
            // Test single bit flips
            for (size_t byte_idx = 0; byte_idx < data_size; ++byte_idx) {
                for (int bit = 0; bit < 8; ++bit) {
                    data[byte_idx] ^= (1 << bit);
                    uint32_t modified = hasher.hash_bytes(data.data(), data.size());
                    
                    // Count differing bits
                    uint32_t diff = original ^ modified;
                    int bits_changed = __builtin_popcount(diff);
                    bit_flip_ratios.push_back(bits_changed / 32.0);
                    
                    data[byte_idx] ^= (1 << bit);  // Restore
                }
            }
        }
        
        // Calculate mean avalanche score
        double mean = std::accumulate(bit_flip_ratios.begin(), 
                                     bit_flip_ratios.end(), 0.0) / bit_flip_ratios.size();
        return mean;
    }
    
    /**
     * @brief Test chi-square distribution
     */
    double test_chi_square(uint64_t table_size, size_t num_samples = DEFAULT_SAMPLES) {
        CrocsHash32 hasher(table_size);
        std::vector<size_t> buckets(table_size, 0);
        
        // Generate samples
        for (size_t i = 0; i < num_samples; ++i) {
            std::string input = "test_" + std::to_string(i) + "_" + std::to_string(rng_());
            uint32_t hash = hasher.hash(input);
            buckets[hash]++;
        }
        
        // Calculate chi-square
        double expected = static_cast<double>(num_samples) / table_size;
        double chi_square = 0;
        
        for (size_t count : buckets) {
            double diff = count - expected;
            chi_square += (diff * diff) / expected;
        }
        
        return chi_square / table_size;  // Normalized
    }
    
    /**
     * @brief Test collision rate
     */
    TestResults::TableSizeResults test_comprehensive(uint64_t table_size, 
                                                     size_t num_samples = DEFAULT_SAMPLES) {
        TestResults::TableSizeResults result;
        result.table_size = table_size;
        
        // Create hasher and get prime
        CrocsHash32 hasher(table_size);
        result.golden_prime = hasher.get_prime();
        
        // Performance test
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t dummy = 0;
        const int perf_iterations = 100000;
        
        for (int i = 0; i < perf_iterations; ++i) {
            std::string key = "perf_test_" + std::to_string(i);
            dummy += hasher.hash(key);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        result.ns_per_hash = static_cast<double>(duration.count()) / perf_iterations;
        
        // Collision test
        std::unordered_map<uint32_t, std::vector<std::string>> hash_map;
        
        for (size_t i = 0; i < num_samples; ++i) {
            std::string key = "collision_test_" + std::to_string(i);
            uint32_t hash = hasher.hash(key);
            hash_map[hash].push_back(key);
        }
        
        result.unique_hashes = hash_map.size();
        result.total_collisions = 0;
        result.max_bucket_load = 0;
        
        for (const auto& [hash, keys] : hash_map) {
            if (keys.size() > 1) {
                result.total_collisions += keys.size() - 1;
            }
            result.max_bucket_load = std::max(result.max_bucket_load, static_cast<uint64_t>(keys.size()));
        }
        
        // Calculate expected collisions
        double n = num_samples;
        double m = table_size;
        double expected_unique = m * (1.0 - std::exp(-n / m));
        double expected_collisions = n - expected_unique;
        
        result.collision_ratio = (expected_collisions > 0) ? 
            result.total_collisions / expected_collisions : 0;
        
        // Distribution tests
        result.chi_square = test_chi_square(table_size, num_samples);
        result.avalanche_score = test_avalanche(table_size);
        
        // Distribution uniformity (0-1, where 1 is perfect)
        double load_factor = static_cast<double>(num_samples) / table_size;
        double expected_max = load_factor * 3;  // Rule of thumb
        result.distribution_uniformity = 1.0 - 
            std::min(1.0, std::abs(result.max_bucket_load - expected_max) / expected_max);
        
        // Prevent optimization
        if (dummy == 0) std::cout << "";
        
        return result;
    }
    
    /**
     * @brief Test multiple table sizes
     */
    TestResults test_multiple_sizes() {
        TestResults results;
        
        // Test sizes from small to very large
        std::vector<uint64_t> test_sizes = {
            // Small sizes
            97, 197, 397, 797,
            // Medium sizes  
            1009, 2003, 4001, 8009,
            10007, 20011, 40009, 80021,
            // Large sizes
            100003, 200003, 400009, 800011,
            1000003, 2000003, 4000037, 8000009,
            // Powers of 2
            256, 512, 1024, 2048, 4096, 8192,
            16384, 32768, 65536, 131072,
            // Near powers of 2
            255, 257, 1023, 1025, 4095, 4097,
            // Powers of 10
            100, 1000, 10000, 100000, 1000000
        };
        
        // Sort for better output
        std::sort(test_sizes.begin(), test_sizes.end());
        
        std::cout << "Testing " << test_sizes.size() << " different table sizes...\n";
        std::cout << "Size      Prime     Chi²   Coll%  Aval   Unif   ns/hash\n";
        std::cout << "--------------------------------------------------------\n";
        
        for (uint64_t size : test_sizes) {
            if (size > 10000000) continue;  // Skip very large for now
            
            auto result = test_comprehensive(size);
            results.results.push_back(result);
            
            std::cout << std::setw(9) << size << " "
                     << std::setw(9) << result.golden_prime << " "
                     << std::fixed << std::setprecision(2)
                     << std::setw(6) << result.chi_square << " "
                     << std::setw(6) << result.collision_ratio * 100 << " "
                     << std::setw(5) << result.avalanche_score << " "
                     << std::setw(5) << result.distribution_uniformity << " "
                     << std::setw(7) << result.ns_per_hash << "\n";
        }
        
        return results;
    }
};

/**
 * @brief Compare CROCS with standard hash functions
 */
class HashComparison {
public:
    // Standard multiplicative hash
    static uint32_t mult_hash(const void* data, size_t len, uint64_t table_size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint64_t hash = 0;
        
        for (size_t i = 0; i < len; ++i) {
            hash = hash * 31 + bytes[i];
        }
        
        return hash % table_size;
    }
    
    // FNV-1a hash
    static uint32_t fnv1a_hash(const void* data, size_t len, uint64_t table_size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint32_t hash = 2166136261u;
        
        for (size_t i = 0; i < len; ++i) {
            hash ^= bytes[i];
            hash *= 16777619u;
        }
        
        return hash % table_size;
    }
    
    // Compare performance and quality
    static void compare_all(uint64_t table_size) {
        std::cout << "\n=== Comparison for table size " << table_size << " ===\n";
        
        const size_t num_samples = 100000;
        std::mt19937 rng(42);
        
        // Test each hash function
        struct HashTest {
            std::string name;
            std::function<uint32_t(const std::string&)> hash_func;
            std::unordered_map<uint32_t, int> distribution;
            double ns_per_hash;
            size_t collisions;
        };
        
        CrocsHash32 crocs_hasher(table_size);
        
        std::vector<HashTest> tests;
        
        // Add CROCS test
        tests.push_back(HashTest{
            "CROCS", 
            [&](const std::string& s) { return crocs_hasher.hash(s); },
            {}, 0.0, 0
        });
        
        // Add Mult31 test
        tests.push_back(HashTest{
            "Mult31",
            [&](const std::string& s) { return mult_hash(s.data(), s.size(), table_size); },
            {}, 0.0, 0
        });
        
        // Add FNV-1a test
        tests.push_back(HashTest{
            "FNV-1a",
            [&](const std::string& s) { return fnv1a_hash(s.data(), s.size(), table_size); },
            {}, 0.0, 0
        });
        
        // Run tests
        for (auto& test : tests) {
            // Performance test
            auto start = std::chrono::high_resolution_clock::now();
            uint64_t dummy = 0;
            
            for (size_t i = 0; i < num_samples; ++i) {
                std::string key = "test_" + std::to_string(i);
                dummy += test.hash_func(key);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            test.ns_per_hash = static_cast<double>(duration.count()) / num_samples;
            
            // Distribution test
            test.distribution.clear();
            for (size_t i = 0; i < num_samples; ++i) {
                std::string key = "dist_" + std::to_string(i);
                test.distribution[test.hash_func(key)]++;
            }
            
            test.collisions = num_samples - test.distribution.size();
            
            if (dummy == 0) std::cout << "";
        }
        
        // Display results
        std::cout << "Hash     ns/hash  Unique  Collisions  Chi²\n";
        std::cout << "-------------------------------------------\n";
        
        for (const auto& test : tests) {
            // Calculate chi-square
            double expected = static_cast<double>(num_samples) / table_size;
            double chi_square = 0;
            
            for (const auto& [hash, count] : test.distribution) {
                double diff = count - expected;
                chi_square += (diff * diff) / expected;
            }
            chi_square /= table_size;
            
            std::cout << std::setw(8) << test.name << " "
                     << std::fixed << std::setprecision(2)
                     << std::setw(7) << test.ns_per_hash << " "
                     << std::setw(7) << test.distribution.size() << " "
                     << std::setw(10) << test.collisions << " "
                     << std::setw(5) << chi_square << "\n";
        }
    }
};

int main() {
    std::cout << "=== CROCS Hash Function Comprehensive Test Suite ===\n\n";
    
    // Run comprehensive tests
    CrocsTester tester;
    auto results = tester.test_multiple_sizes();
    
    // Save results
    results.save_csv("../crocs_test_results.csv");
    std::cout << "\nResults saved to ../crocs_test_results.csv\n";
    
    // Run comparisons for select sizes
    std::cout << "\n=== Hash Function Comparisons ===\n";
    std::vector<uint64_t> comparison_sizes = {1009, 10007, 100003, 1024, 65536};
    
    for (uint64_t size : comparison_sizes) {
        HashComparison::compare_all(size);
    }
    
    return 0;
}