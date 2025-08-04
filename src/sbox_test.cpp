#include <goldenhash.hpp>
#include <iostream>
#include <vector>
#include <bitset>

int main() {
    std::cout << "S-Box Analysis for Different Table Sizes\n";
    std::cout << "========================================\n";
    
    // Test with different N values - including very large ones
    std::vector<uint64_t> test_sizes = {
        1048576,             // 2^20 (baseline)
        4294967296
    };

    // Add about 10 more random sizes between 1M and 4B
    for (uint64_t i = 0; i < 10; i++) {
        uint64_t size = 1048576 + (rand() % (4294967296 - 1048576));
        test_sizes.push_back(size);
    }
    
    for (uint64_t N : test_sizes) {
        goldenhash::GoldenHash hasher(N);
        
        // Show binary values for this configuration
        uint64_t prime_high = hasher.get_prime_high();
        uint64_t prime_low = hasher.get_prime_low();
        uint64_t prime_product = hasher.get_prime_product();
        uint64_t prime_mod = hasher.get_prime_mod();
        uint64_t working_mod = hasher.get_working_mod();
        uint64_t prime_mixed = hasher.get_prime_mixed();
        uint64_t initial_hash = hasher.get_initial_hash();
        std::vector<uint64_t> factors = hasher.get_factors();
        
        std::cout << "\nTABLE SIZE: " << N << "\n";
        std::cout << "  prime_high:    " << std::bitset<64>(prime_high) << " (" << prime_high << ")\n";
        std::cout << "  prime_low:     " << std::bitset<64>(prime_low) << " (" << prime_low << ")\n";
        std::cout << "  prime_product: " << std::bitset<64>(prime_product) << " (" << prime_product << ")\n";
        std::cout << "  prime_mod:     " << std::bitset<64>(prime_mod) << " (" << prime_mod << ")\n";
        std::cout << "  working_mod:   " << std::bitset<64>(working_mod) << " (" << working_mod << ")\n";
        std::cout << "  prime_mixed:   " << std::bitset<64>(prime_mixed) << " (" << prime_mixed << ")\n";
        std::cout << "  initial_hash:  " << std::bitset<64>(initial_hash) << " (" << initial_hash << ")\n";
        std::cout << "  factors:       ";
        for (size_t i = 0; i < factors.size(); i++) {
            std::cout << factors[i];
            if (i < factors.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        hasher.analyze_sboxes();
        std::cout << "----------------------------------------\n\n";
    }
    
    return 0;
}