/**
 * @file hash64_demo.cpp
 * @brief Demonstration of 64-bit hash collision analysis
 * @author Josh Morgan
 * @date 2025
 * 
 * This program demonstrates the birthday paradox for different hash sizes
 * and shows how to use the Hash64Analyzer for large-scale testing.
 */

#include <goldenhash.hpp>
#include <goldenhash/tests/hash64_analyzer.hpp>
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>

using namespace goldenhash;
using namespace goldenhash::tests;

int main() {
    std::cout << "\n=== 64-bit Hash Collision Analysis Demo ===\n\n";
    
    // Show birthday paradox table
    std::cout << BirthdayParadoxCalculator::get_probability_table() << "\n";
    
    // Demonstrate with small hash space (16-bit) to show collisions
    std::cout << "\n=== Testing with 16-bit hash space ===\n";
    std::cout << "Expected: ~302 hashes for 50% collision probability\n\n";
    
    // Create a simple 16-bit hash function
    auto hash16 = [](const std::string& s) -> uint16_t {
        GoldenHash hasher(65536);  // 2^16
        std::vector<uint8_t> data(s.begin(), s.end());
        return hasher.hash(data.data(), data.size()) & 0xFFFF;
    };
    
    // Test with increasing numbers of hashes
    std::mt19937 rng(42);
    std::uniform_int_distribution<> dist(100000, 999999);
    
    for (int test_size : {100, 200, 300, 400, 500, 1000}) {
        std::unordered_set<uint16_t> seen;
        int collisions = 0;
        
        for (int i = 0; i < test_size; i++) {
            std::string input = std::to_string(dist(rng)) + "_" + std::to_string(i);
            uint16_t hash = hash16(input);
            
            if (seen.count(hash) > 0) {
                collisions++;
            } else {
                seen.insert(hash);
            }
        }
        
        double expected_collisions = (static_cast<double>(test_size) * test_size) / (2.0 * 65536);
        
        std::cout << "Hashes: " << std::setw(5) << test_size 
                  << " | Collisions: " << std::setw(3) << collisions
                  << " | Expected: " << std::fixed << std::setprecision(1) << expected_collisions
                  << " | Probability: " << std::fixed << std::setprecision(3) 
                  << (1.0 - std::exp(-(static_cast<double>(test_size) * test_size) / (2.0 * 65536)))
                  << "\n";
    }
    
    // Demonstrate Hash64Analyzer
    std::cout << "\n=== Hash64Analyzer Demo ===\n";
    std::cout << "Simulating analysis of 10,000 64-bit hashes...\n\n";
    
    Hash64Analyzer analyzer;
    GoldenHash hasher(UINT64_MAX);
    
    // Simulate adding hashes
    for (int i = 0; i < 10000; i++) {
        std::string input = "test_" + std::to_string(i);
        std::vector<uint8_t> data(input.begin(), input.end());
        uint64_t hash = hasher.hash(data.data(), data.size());
        analyzer.add_hash(hash, data.data(), data.size());
    }
    
    std::cout << analyzer.get_statistics() << "\n";
    
    // Show collision probability for different numbers of 64-bit hashes
    std::cout << "\n=== Collision Probability for 64-bit Hashes ===\n";
    std::cout << "Number of hashes | Collision probability\n";
    std::cout << "-----------------|----------------------\n";
    
    for (uint64_t n : {1000ULL, 10000ULL, 100000ULL, 1000000ULL, 10000000ULL, 
                      100000000ULL, 1000000000ULL, 5000000000ULL}) {
        double prob = 1.0 - std::exp(-(static_cast<double>(n) * n) / std::pow(2.0, 65));
        
        std::cout << std::setw(16);
        if (n >= 1000000000) {
            std::cout << std::fixed << std::setprecision(1) << (n / 1e9) << "B";
        } else if (n >= 1000000) {
            std::cout << std::fixed << std::setprecision(1) << (n / 1e6) << "M";
        } else {
            std::cout << n;
        }
        
        std::cout << " | " << std::scientific << std::setprecision(2) << prob << "\n";
    }
    
    std::cout << "\nFor comparison: 5.06 billion hashes = 50% collision probability\n";
    
    return 0;
}