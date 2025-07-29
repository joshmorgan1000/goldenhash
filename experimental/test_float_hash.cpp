/**
 * @file test_float_hash.cpp
 * @brief Test program for the float_hash function from CLAUDE.md
 * @details This program tests the deterministic floating-point based golden ratio hash
 */

#include <cfenv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <vector>

double golden_ratio = 1.6180339887498948482;

uint64_t float_hash(uint64_t key, uint64_t seed, uint64_t table_size) {
    // Ensure deterministic rounding
    std::fesetround(FE_TONEAREST);

    double x = static_cast<double>(key ^ seed);
    double mix = x * golden_ratio * static_cast<double>(table_size);
    
    // Map into [0, 1) range deterministically
    double frac = mix - std::floor(mix);
    
    // Scale to integer space (e.g., 64-bit)
    return static_cast<uint64_t>(frac * 0xFFFFFFFFFFFFFFFFull);
}

int main() {
    std::cout << "Testing float_hash function with golden ratio: " << golden_ratio << "\n\n";
    
    // Test with different table sizes
    std::vector<uint64_t> table_sizes = {16, 32, 64, 128, 256, 1024, 4096};
    std::vector<uint64_t> test_keys = {0, 1, 2, 3, 42, 100, 1000, 0xDEADBEEF, 0xCAFEBABE};
    uint64_t seed = 0x1337;
    
    for (auto table_size : table_sizes) {
        std::cout << "Table size: " << table_size << "\n";
        std::cout << std::string(60, '-') << "\n";
        std::cout << std::setw(12) << "Key" << std::setw(20) << "Hash Value" 
                  << std::setw(20) << "Hash % Table" << "\n";
        
        for (auto key : test_keys) {
            uint64_t hash = float_hash(key, seed, table_size);
            uint64_t index = hash % table_size;
            
            std::cout << std::setw(12) << std::hex << key 
                      << std::setw(20) << hash
                      << std::setw(20) << std::dec << index << "\n";
        }
        std::cout << "\n";
    }
    
    // Test determinism
    std::cout << "Testing determinism (same key should produce same hash):\n";
    for (int i = 0; i < 5; ++i) {
        uint64_t hash = float_hash(42, seed, 1024);
        std::cout << "  Iteration " << i << ": " << std::hex << hash << "\n";
    }
    
    // Test sensitivity to table size
    std::cout << "\nTesting sensitivity to table size (key=42, seed=0x1337):\n";
    for (auto table_size : table_sizes) {
        uint64_t hash = float_hash(42, seed, table_size);
        std::cout << "  Table size " << std::dec << std::setw(5) << table_size 
                  << ": hash = " << std::hex << hash << "\n";
    }
    
    return 0;
}