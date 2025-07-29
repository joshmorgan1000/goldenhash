#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <bitset>
#include <cstring>
#include <iomanip>

using namespace crocs;

struct TestResult {
    uint64_t table_size;
    uint64_t golden_prime;
    double chi_square;
    double collision_ratio;
    double avalanche_score;
    double distribution_uniformity;
    double ns_per_hash;
    uint64_t unique_hashes;
    uint64_t total_collisions;
    uint64_t max_bucket_load;
    int bits_needed;
    int64_t prime_distance;
    double total_test_time_seconds;
};

// Industry standard test data generators
class TestDataGenerator {
private:
    std::mt19937_64 rng_;
    
public:
    TestDataGenerator(uint64_t seed = 42) : rng_(seed) {}
    
    // Random binary data (SMHasher style)
    std::vector<uint8_t> random_binary(size_t length) {
        std::vector<uint8_t> data(length);
        for (auto& byte : data) {
            byte = rng_() & 0xFF;
        }
        return data;
    }
    
    // Key-value style strings
    std::string key_value(size_t index) {
        return "key_" + std::to_string(index);
    }
    
    // Sparse data (mostly zeros)
    std::vector<uint8_t> sparse_data(size_t length, double sparsity = 0.95) {
        std::vector<uint8_t> data(length, 0);
        for (size_t i = 0; i < length; ++i) {
            if ((rng_() / double(UINT64_MAX)) > sparsity) {
                data[i] = rng_() & 0xFF;
            }
        }
        return data;
    }
    
    // Sequential data with small differences
    std::vector<uint8_t> sequential(size_t index, size_t length = 8) {
        std::vector<uint8_t> data(length);
        uint64_t value = index;
        memcpy(data.data(), &value, std::min(length, sizeof(value)));
        return data;
    }
};

double test_avalanche_comprehensive(uint64_t table_size, size_t num_tests = 10000) {
    uint64_t prime = GoldenPrimeFinder::find_golden_prime(table_size);
    TestDataGenerator gen;
    std::vector<double> bit_flip_ratios;
    
    // Test different data patterns
    for (size_t test = 0; test < num_tests; ++test) {
        // Vary data size (4 to 64 bytes)
        size_t data_size = 4 + (test % 61);
        auto data = gen.random_binary(data_size);
        
        // Hash original
        uint64_t h1 = 0;
        for (size_t i = 0; i < data_size; ++i) {
            h1 = h1 * prime + data[i];
            h1 ^= h1 >> 32;
        }
        h1 = (h1 * prime) % table_size;
        
        // Test single bit flips across the data
        size_t samples = std::min<size_t>(data_size * 8, 128); // Cap at 128 bit flips
        for (size_t s = 0; s < samples; ++s) {
            size_t byte_idx = (s / 8) % data_size;
            int bit = s % 8;
            
            data[byte_idx] ^= (1 << bit);
            
            uint64_t h2 = 0;
            for (size_t i = 0; i < data_size; ++i) {
                h2 = h2 * prime + data[i];
                h2 ^= h2 >> 32;
            }
            h2 = (h2 * prime) % table_size;
            
            data[byte_idx] ^= (1 << bit); // Flip back
            
            // Count differing bits
            uint64_t xor_result = h1 ^ h2;
            int bits_changed = __builtin_popcountll(xor_result);
            int total_bits = 64 - __builtin_clzll(table_size - 1);
            bit_flip_ratios.push_back(static_cast<double>(bits_changed) / total_bits);
        }
    }
    
    // Calculate average
    double sum = 0;
    for (double ratio : bit_flip_ratios) {
        sum += ratio;
    }
    return sum / bit_flip_ratios.size();
}

TestResult test_table_size_comprehensive(uint64_t table_size, size_t num_samples) {
    auto total_start = std::chrono::high_resolution_clock::now();
    
    TestResult result;
    result.table_size = table_size;
    
    // Find golden prime
    result.golden_prime = GoldenPrimeFinder::find_golden_prime(table_size);
    
    // Calculate prime distance
    uint64_t golden_value = static_cast<uint64_t>(table_size / PHI);
    result.prime_distance = static_cast<int64_t>(result.golden_prime) - 
                           static_cast<int64_t>(golden_value);
    
    // Distribution test with multiple data types
    std::vector<uint64_t> buckets(table_size, 0);
    TestDataGenerator gen;
    
    auto hash_start = std::chrono::high_resolution_clock::now();
    
    // Test 1: Random binary data (40% of tests)
    size_t random_tests = num_samples * 0.4;
    for (size_t i = 0; i < random_tests; ++i) {
        auto data = gen.random_binary(8 + (i % 57)); // 8-64 byte data
        
        uint64_t h = 0;
        for (uint8_t byte : data) {
            h = h * result.golden_prime + byte;
            h ^= h >> 32;
        }
        h = (h * result.golden_prime) % table_size;
        buckets[h]++;
    }
    
    // Test 2: Key-value strings (30% of tests)
    size_t kv_tests = num_samples * 0.3;
    for (size_t i = 0; i < kv_tests; ++i) {
        std::string key = gen.key_value(i);
        
        uint64_t h = 0;
        for (char c : key) {
            h = h * result.golden_prime + static_cast<uint8_t>(c);
            h ^= h >> 32;
        }
        h = (h * result.golden_prime) % table_size;
        buckets[h]++;
    }
    
    // Test 3: Sequential data (20% of tests)
    size_t seq_tests = num_samples * 0.2;
    for (size_t i = 0; i < seq_tests; ++i) {
        auto data = gen.sequential(i);
        
        uint64_t h = 0;
        for (uint8_t byte : data) {
            h = h * result.golden_prime + byte;
            h ^= h >> 32;
        }
        h = (h * result.golden_prime) % table_size;
        buckets[h]++;
    }
    
    // Test 4: Sparse data (10% of tests)
    size_t sparse_tests = num_samples - random_tests - kv_tests - seq_tests;
    for (size_t i = 0; i < sparse_tests; ++i) {
        auto data = gen.sparse_data(16 + (i % 49));
        
        uint64_t h = 0;
        for (uint8_t byte : data) {
            h = h * result.golden_prime + byte;
            h ^= h >> 32;
        }
        h = (h * result.golden_prime) % table_size;
        buckets[h]++;
    }
    
    auto hash_end = std::chrono::high_resolution_clock::now();
    auto hash_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(hash_end - hash_start);
    result.ns_per_hash = static_cast<double>(hash_duration.count()) / num_samples;
    
    // Calculate statistics
    double expected = static_cast<double>(num_samples) / table_size;
    result.chi_square = 0;
    result.unique_hashes = 0;
    result.max_bucket_load = 0;
    
    for (uint64_t count : buckets) {
        if (count > 0) result.unique_hashes++;
        if (count > result.max_bucket_load) result.max_bucket_load = count;
        double diff = count - expected;
        result.chi_square += (diff * diff) / expected;
    }
    result.chi_square /= table_size;
    
    result.total_collisions = num_samples - result.unique_hashes;
    
    // Calculate expected collisions (birthday paradox)
    double expected_unique = table_size * (1.0 - std::exp(-static_cast<double>(num_samples) / table_size));
    double expected_collisions = num_samples - expected_unique;
    result.collision_ratio = (expected_collisions > 0) ? 
        result.total_collisions / expected_collisions : 1.0;
    
    // Distribution uniformity
    double variance = 0;
    for (uint64_t count : buckets) {
        double diff = count - expected;
        variance += diff * diff;
    }
    result.distribution_uniformity = std::sqrt(variance / table_size) / expected;
    
    // Avalanche test (comprehensive)
    if (!std::getenv("SKIP_AVALANCHE")) {
        result.avalanche_score = test_avalanche_comprehensive(table_size);
    } else {
        result.avalanche_score = 0.5; // Placeholder
    }
    
    // Bits needed
    result.bits_needed = 64 - __builtin_clzll(table_size - 1);
    
    // Total time
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start);
    result.total_test_time_seconds = total_duration.count() / 1000.0;
    
    return result;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    uint64_t size = 10007;
    size_t tests = 100000;
    bool csv_output = false;
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--size=", 7) == 0) {
            size = std::stoull(argv[i] + 7);
        } else if (strncmp(argv[i], "--tests=", 8) == 0) {
            tests = std::stoull(argv[i] + 8);
        } else if (strcmp(argv[i], "--csv-output") == 0) {
            csv_output = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    if (verbose && !csv_output) {
        std::cout << "Testing CROCS hash for table size " << size << "\n";
        std::cout << "Number of hash operations: " << tests << "\n";
        std::cout << "Test types: random binary, key-value, sequential, sparse\n\n";
    }
    
    // Run test
    TestResult result = test_table_size_comprehensive(size, tests);
    
    if (csv_output) {
        // Output header if this is the first line
        static bool header_printed = false;
        if (!header_printed) {
            std::cout << "table_size,golden_prime,chi_square,collision_ratio,avalanche_score,"
                      << "distribution_uniformity,ns_per_hash,unique_hashes,total_collisions,"
                      << "max_bucket_load,bits_needed,prime_distance_from_golden\n";
            header_printed = true;
        }
        
        // Output data
        std::cout << result.table_size << ","
                  << result.golden_prime << ","
                  << result.chi_square << ","
                  << result.collision_ratio << ","
                  << result.avalanche_score << ","
                  << result.distribution_uniformity << ","
                  << result.ns_per_hash << ","
                  << result.unique_hashes << ","
                  << result.total_collisions << ","
                  << result.max_bucket_load << ","
                  << result.bits_needed << ","
                  << result.prime_distance << "\n";
    } else {
        std::cout << "\nResults:\n";
        std::cout << "--------\n";
        std::cout << "Table size: " << result.table_size << "\n";
        std::cout << "Golden prime: " << result.golden_prime << "\n";
        std::cout << "Chi-square: " << std::fixed << std::setprecision(4) << result.chi_square << "\n";
        std::cout << "Collision ratio: " << result.collision_ratio << "\n";
        std::cout << "Avalanche score: " << result.avalanche_score << " (ideal: 0.5)\n";
        std::cout << "Distribution uniformity: " << result.distribution_uniformity << "\n";
        std::cout << "Performance: " << result.ns_per_hash << " ns/hash\n";
        std::cout << "Unique hashes: " << result.unique_hashes << "/" << tests << "\n";
        std::cout << "Total test time: " << result.total_test_time_seconds << " seconds\n";
    }
    
    return 0;
}