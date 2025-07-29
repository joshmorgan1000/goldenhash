#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <bitset>
#include <cstring>

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
};

double test_avalanche(uint64_t table_size, size_t num_tests = 1000) {
    uint64_t prime = GoldenPrimeFinder::find_golden_prime(table_size);
    
    std::mt19937 rng(42);
    const size_t data_size = 16;
    std::vector<double> bit_flip_ratios;
    
    for (size_t i = 0; i < num_tests; ++i) {
        // Generate random data
        std::vector<uint8_t> data(data_size);
        for (auto& byte : data) {
            byte = rng() & 0xFF;
        }
        
        // Hash original
        uint64_t h1 = 0;
        for (size_t j = 0; j < data_size; ++j) {
            h1 = h1 * prime + data[j];
            h1 ^= h1 >> 32;
        }
        h1 = (h1 * prime) % table_size;
        
        // Test single bit flips
        for (size_t byte_idx = 0; byte_idx < data_size; ++byte_idx) {
            for (int bit = 0; bit < 8; ++bit) {
                data[byte_idx] ^= (1 << bit);
                
                uint64_t h2 = 0;
                for (size_t j = 0; j < data_size; ++j) {
                    h2 = h2 * prime + data[j];
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
    }
    
    // Average bit flip ratio
    double sum = 0;
    for (double ratio : bit_flip_ratios) {
        sum += ratio;
    }
    return sum / bit_flip_ratios.size();
}

TestResult test_table_size(uint64_t table_size, size_t num_samples) {
    TestResult result;
    result.table_size = table_size;
    
    // Find golden prime
    result.golden_prime = GoldenPrimeFinder::find_golden_prime(table_size);
    
    // Calculate prime distance
    uint64_t golden_value = static_cast<uint64_t>(table_size / PHI);
    result.prime_distance = static_cast<int64_t>(result.golden_prime) - 
                           static_cast<int64_t>(golden_value);
    
    // Distribution test
    std::vector<uint64_t> buckets(table_size, 0);
    std::mt19937_64 rng(42);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < num_samples; ++i) {
        uint64_t value = rng();
        
        // Simple CROCS hash
        uint64_t h = 0;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
        for (size_t j = 0; j < sizeof(value); ++j) {
            h = h * result.golden_prime + bytes[j];
            h ^= h >> 32;
        }
        h = (h * result.golden_prime) % table_size;
        
        buckets[h]++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    result.ns_per_hash = static_cast<double>(duration.count()) / num_samples;
    
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
    
    // Distribution uniformity (0 = perfectly uniform)
    double variance = 0;
    for (uint64_t count : buckets) {
        double diff = count - expected;
        variance += diff * diff;
    }
    result.distribution_uniformity = std::sqrt(variance / table_size) / expected;
    
    // Avalanche test
    result.avalanche_score = test_avalanche(table_size);
    
    // Bits needed
    result.bits_needed = 64 - __builtin_clzll(table_size - 1);
    
    return result;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    uint64_t size = 10007;
    size_t tests = 100000;
    bool csv_output = false;
    
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--size=", 7) == 0) {
            size = std::stoull(argv[i] + 7);
        } else if (strncmp(argv[i], "--tests=", 8) == 0) {
            tests = std::stoull(argv[i] + 8);
        } else if (strcmp(argv[i], "--csv-output") == 0) {
            csv_output = true;
        }
    }
    
    // Run test
    TestResult result = test_table_size(size, tests);
    
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
        std::cout << "Table size: " << result.table_size << "\n";
        std::cout << "Golden prime: " << result.golden_prime << "\n";
        std::cout << "Chi-square: " << result.chi_square << "\n";
        std::cout << "Collision ratio: " << result.collision_ratio << "\n";
        std::cout << "Avalanche score: " << result.avalanche_score << "\n";
        std::cout << "Performance: " << result.ns_per_hash << " ns/hash\n";
    }
    
    return 0;
}