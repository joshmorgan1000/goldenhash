/**
 * @file test_float_hash_metrics.cpp
 * @brief Test program for FloatHash using HashMetrics framework
 * @author Josh Morgan
 * @date 2025
 * 
 * This program tests the FloatHash implementation using the HashMetrics
 * framework and outputs results in JSON format.
 */

#include "float_hash.hpp"
#include "hash_metrics.hpp"
#include <iostream>
#include <fstream>
#include <vector>

using namespace goldenhash;

/**
 * @brief Run comprehensive tests on FloatHash
 */
void run_comprehensive_tests() {
    FloatHash float_hash;
    HashMetrics metrics;
    
    // Test configurations
    std::vector<TestConfiguration> configs = {
        // Small table test
        {.num_keys = 10000, .table_size = 128, .seed = 42},
        
        // Medium table test
        {.num_keys = 100000, .table_size = 1024, .seed = 12345},
        
        // Large table test
        {.num_keys = 1000000, .table_size = 65536, .seed = 0xDEADBEEF},
        
        // Power of 2 table sizes
        {.num_keys = 100000, .table_size = 256, .seed = 0},
        {.num_keys = 100000, .table_size = 512, .seed = 0},
        {.num_keys = 100000, .table_size = 2048, .seed = 0},
        
        // Prime table sizes
        {.num_keys = 100000, .table_size = 127, .seed = 0},
        {.num_keys = 100000, .table_size = 1021, .seed = 0},
        {.num_keys = 100000, .table_size = 65521, .seed = 0}
    };
    
    std::cout << "Running FloatHash tests...\n\n";
    
    for (size_t i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        std::cout << "Test " << (i + 1) << "/" << configs.size() 
                  << " - Keys: " << config.num_keys 
                  << ", Table Size: " << config.table_size 
                  << ", Seed: " << config.seed << "\n";
        
        auto results = metrics.run_tests(float_hash, config);
        
        // Output results summary
        for (const auto& result : results) {
            std::cout << "  " << result.name << ": " << result.value 
                      << " " << result.unit << "\n";
        }
        
        // Save detailed results to JSON file
        std::string filename = "float_hash_results_" + std::to_string(i + 1) + ".json";
        std::ofstream outfile(filename);
        outfile << metrics.export_to_json(results, float_hash, config);
        outfile.close();
        
        std::cout << "  Results saved to: " << filename << "\n\n";
    }
}

/**
 * @brief Demonstrate basic FloatHash usage
 */
void demonstrate_basic_usage() {
    std::cout << "=== FloatHash Basic Usage Demo ===\n\n";
    
    FloatHash hash;
    
    // Test some specific values
    std::vector<uint64_t> test_keys = {0, 1, 42, 0xDEADBEEF, UINT64_MAX};
    uint64_t table_size = 1024;
    uint64_t seed = 12345;
    
    std::cout << "Hash function: " << hash.get_name() << "\n";
    std::cout << "Description: " << hash.get_description() << "\n";
    std::cout << "Table size: " << table_size << "\n";
    std::cout << "Seed: " << seed << "\n\n";
    
    std::cout << "Key -> Hash (no seed) -> Hash (with seed)\n";
    std::cout << "----------------------------------------\n";
    
    for (const auto& key : test_keys) {
        uint64_t hash_no_seed = hash.hash(key, table_size);
        uint64_t hash_with_seed = hash.hash_with_seed(key, seed, table_size);
        
        std::cout << std::hex << "0x" << key << " -> " 
                  << std::dec << hash_no_seed << " -> " 
                  << hash_with_seed << "\n";
    }
    
    std::cout << "\n";
}

/**
 * @brief Compare hash values for different table sizes
 */
void demonstrate_table_size_sensitivity() {
    std::cout << "=== Table Size Sensitivity Demo ===\n\n";
    
    FloatHash hash;
    uint64_t key = 0xABCDEF123456789ULL;
    uint64_t seed = 0;
    
    std::vector<uint64_t> table_sizes = {16, 32, 64, 128, 256, 512, 1024, 2048};
    
    std::cout << "Key: 0x" << std::hex << key << std::dec << "\n";
    std::cout << "Demonstrating that hash(k, N1) != hash(k, N2)\n\n";
    
    std::cout << "Table Size -> Hash Value\n";
    std::cout << "------------------------\n";
    
    for (const auto& size : table_sizes) {
        uint64_t hash_val = hash.hash_with_seed(key, seed, size);
        std::cout << size << " -> " << hash_val << "\n";
    }
    
    std::cout << "\n";
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    try {
        // Basic demonstrations
        demonstrate_basic_usage();
        demonstrate_table_size_sensitivity();
        
        // Run comprehensive tests
        run_comprehensive_tests();
        
        std::cout << "All tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}