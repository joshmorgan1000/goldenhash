#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <iomanip>

using namespace crocs;

// Test configuration from command line
struct Config {
    uint64_t table_size = 10007;
    size_t num_operations = 1000000;
    size_t data_size = 64;
    bool verbose = false;
    bool csv_output = false;
    std::string test_type = "standard";
};

// Comprehensive test that actually does work
void run_comprehensive_test(const Config& config) {
    if (config.verbose) {
        std::cout << "CROCS Comprehensive Test\n";
        std::cout << "Table size: " << config.table_size << "\n";
        std::cout << "Operations: " << config.num_operations << "\n";
        std::cout << "Data size: " << config.data_size << " bytes\n\n";
    }
    
    // Find golden prime
    uint64_t prime = GoldenPrimeFinder::find_golden_prime(config.table_size);
    
    // For huge tables, use sampling instead of full allocation
    bool use_sampling = config.table_size > 1000000000;
    std::vector<uint64_t> buckets;
    std::unordered_map<uint64_t, uint64_t> sampled_buckets;
    
    if (!use_sampling) {
        buckets.resize(config.table_size, 0);
    }
    
    // Generate test data
    std::mt19937_64 rng(42);
    std::vector<std::vector<uint8_t>> test_data;
    
    // Pre-generate all test data to ensure consistent timing
    for (size_t i = 0; i < config.num_operations; i++) {
        std::vector<uint8_t> data(config.data_size);
        for (auto& byte : data) {
            byte = rng() & 0xFF;
        }
        test_data.push_back(data);
    }
    
    if (config.verbose) {
        std::cout << "Generated " << test_data.size() << " test inputs\n";
        std::cout << "Starting hash operations...\n";
    }
    
    // Run the actual hashing
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& data : test_data) {
        // CROCS hash computation
        uint64_t h = 0;
        
        // Process each byte
        for (uint8_t byte : data) {
            h = h * prime + byte;
            h ^= h >> 32;
        }
        
        // Final mix
        h = h * prime;
        h ^= h >> 32;
        h = h % config.table_size;
        
        // Record result
        if (use_sampling) {
            sampled_buckets[h]++;
        } else {
            buckets[h]++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double ns_per_hash = (duration.count() * 1000000.0) / config.num_operations;
    
    // Calculate statistics
    uint64_t unique_hashes = 0;
    uint64_t max_bucket = 0;
    double chi_square = 0.0;
    
    if (use_sampling) {
        unique_hashes = sampled_buckets.size();
        for (const auto& [hash, count] : sampled_buckets) {
            if (count > max_bucket) max_bucket = count;
        }
        // Chi-square approximation for sampled data
        double expected = (double)config.num_operations / config.table_size;
        for (const auto& [hash, count] : sampled_buckets) {
            double diff = count - expected;
            chi_square += (diff * diff) / expected;
        }
        chi_square = chi_square * config.table_size / sampled_buckets.size();
    } else {
        double expected = (double)config.num_operations / config.table_size;
        for (uint64_t count : buckets) {
            if (count > 0) unique_hashes++;
            if (count > max_bucket) max_bucket = count;
            double diff = count - expected;
            chi_square += (diff * diff) / expected;
        }
        chi_square /= config.table_size;
    }
    
    uint64_t collisions = config.num_operations - unique_hashes;
    
    // Avalanche test (on subset for performance)
    double avalanche_score = 0.0;
    if (!config.csv_output) {
        size_t avalanche_tests = std::min<size_t>(1000, config.num_operations / 100);
        size_t bit_changes = 0;
        size_t total_bits = 0;
        
        for (size_t i = 0; i < avalanche_tests; i++) {
            auto data = test_data[i];
            
            // Original hash
            uint64_t h1 = 0;
            for (uint8_t byte : data) {
                h1 = h1 * prime + byte;
                h1 ^= h1 >> 32;
            }
            h1 = (h1 * prime) % config.table_size;
            
            // Flip each bit
            for (size_t byte_idx = 0; byte_idx < data.size(); byte_idx++) {
                for (int bit = 0; bit < 8; bit++) {
                    data[byte_idx] ^= (1 << bit);
                    
                    uint64_t h2 = 0;
                    for (uint8_t byte : data) {
                        h2 = h2 * prime + byte;
                        h2 ^= h2 >> 32;
                    }
                    h2 = (h2 * prime) % config.table_size;
                    
                    data[byte_idx] ^= (1 << bit);
                    
                    uint64_t diff = h1 ^ h2;
                    bit_changes += __builtin_popcountll(diff);
                    total_bits += 64;
                }
            }
        }
        
        avalanche_score = (double)bit_changes / total_bits;
    }
    
    // Output results
    if (config.csv_output) {
        std::cout << config.table_size << ","
                  << prime << ","
                  << chi_square << ","
                  << collisions << ","
                  << unique_hashes << ","
                  << ns_per_hash << ","
                  << seconds << "\n";
    } else {
        std::cout << "\nResults:\n";
        std::cout << "==========================================\n";
        std::cout << "Table size:        " << config.table_size << "\n";
        std::cout << "Golden prime:      " << prime << "\n";
        std::cout << "Golden ratio:      " << (double)config.table_size / prime << " (φ = " << PHI << ")\n";
        std::cout << "Total time:        " << seconds << " seconds\n";
        std::cout << "Performance:       " << ns_per_hash << " ns/hash\n";
        std::cout << "Throughput:        " << (config.num_operations / seconds / 1000000) << " M ops/sec\n";
        std::cout << "Unique hashes:     " << unique_hashes << "\n";
        std::cout << "Collisions:        " << collisions << "\n";
        std::cout << "Chi-square:        " << chi_square << " (ideal: 1.0)\n";
        std::cout << "Max bucket:        " << max_bucket << "\n";
        std::cout << "Avalanche:         " << avalanche_score << " (ideal: 0.5)\n";
    }
}

Config parse_args(int argc, char* argv[]) {
    Config config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--size" && i + 1 < argc) {
            config.table_size = std::stoull(argv[++i]);
        } else if (arg == "--ops" && i + 1 < argc) {
            config.num_operations = std::stoull(argv[++i]);
        } else if (arg == "--data-size" && i + 1 < argc) {
            config.data_size = std::stoull(argv[++i]);
        } else if (arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--csv") {
            config.csv_output = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "  --size N        Table size (default: 10007)\n";
            std::cout << "  --ops N         Number of operations (default: 1000000)\n";
            std::cout << "  --data-size N   Size of data to hash in bytes (default: 64)\n";
            std::cout << "  --verbose       Verbose output\n";
            std::cout << "  --csv           CSV output format\n";
            std::exit(0);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    Config config = parse_args(argc, argv);
    
    if (!config.csv_output) {
        std::cout << "CROCS Hash Function Test\n";
        std::cout << "========================\n";
    }
    
    run_comprehensive_test(config);
    
    return 0;
}