#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cstring>
#include <chrono>
#include "include/crocs.hpp"

using namespace crocs;

int main(int argc, char* argv[]) {
    // Parse command line arguments
    size_t table_size = 10007;
    size_t test_count = 100000;
    bool quiet = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--table-size") == 0 && i + 1 < argc) {
            table_size = std::stoull(argv[++i]);
        } else if (strcmp(argv[i], "--test-count") == 0 && i + 1 < argc) {
            test_count = std::stoull(argv[++i]);
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        }
    }
    
    // Find golden prime
    auto prime_finder = GoldenPrimeFinder<size_t>::for_table_size(table_size);
    auto golden_prime = prime_finder.find_prime();
    
    if (!quiet) {
        std::cout << "Testing CROCS for table size " << table_size << std::endl;
        std::cout << "Golden prime: " << golden_prime << std::endl;
    }
    
    // Create hash function
    auto hash = CrocsHash<size_t>::for_table_size(table_size);
    
    // Run distribution test
    std::vector<size_t> buckets(table_size, 0);
    std::mt19937_64 rng(42);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < test_count; i++) {
        uint64_t value = rng();
        size_t h = hash(reinterpret_cast<uint8_t*>(&value), sizeof(value));
        buckets[h % table_size]++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double ns_per_hash = static_cast<double>(duration.count()) / test_count;
    
    // Calculate chi-square
    double expected = static_cast<double>(test_count) / table_size;
    double chi_square = 0.0;
    size_t unique_hashes = 0;
    size_t max_bucket = 0;
    
    for (size_t count : buckets) {
        if (count > 0) unique_hashes++;
        if (count > max_bucket) max_bucket = count;
        double diff = count - expected;
        chi_square += (diff * diff) / expected;
    }
    chi_square /= table_size;
    
    // Calculate collision metrics
    size_t total_collisions = test_count - unique_hashes;
    double expected_unique = table_size * (1.0 - std::exp(-static_cast<double>(test_count) / table_size));
    double expected_collisions = test_count - expected_unique;
    double collision_ratio = total_collisions / expected_collisions;
    
    // Output in parseable format
    std::cout << "Chi-square: " << chi_square << std::endl;
    std::cout << "Collision ratio: " << collision_ratio << std::endl;
    std::cout << "Performance: " << ns_per_hash << " ns/hash" << std::endl;
    std::cout << "Unique hashes: " << unique_hashes << std::endl;
    std::cout << "Total collisions: " << total_collisions << std::endl;
    std::cout << "Golden prime: " << golden_prime << std::endl;
    
    return 0;
}