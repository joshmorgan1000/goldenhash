#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <unordered_map>
#include <cmath>

using namespace crocs;

// Test huge table sizes without allocating full arrays
void test_huge_table(uint64_t table_size, size_t sample_size = 1000000) {
    std::cout << "\nTesting table size: " << table_size << " (2^" << log2(table_size) << ")\n";
    
    // Find golden prime
    uint64_t prime = GoldenPrimeFinder::find_golden_prime(table_size);
    uint64_t golden_ideal = table_size / PHI;
    
    std::cout << "Golden prime: " << prime << "\n";
    std::cout << "Golden ideal: " << golden_ideal << "\n";
    std::cout << "Prime ratio: " << (double)table_size / prime << " (φ = " << PHI << ")\n";
    
    // Sample-based collision testing
    std::unordered_map<uint64_t, uint32_t> sampled_buckets;
    std::mt19937_64 rng(42);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < sample_size; i++) {
        uint64_t value = rng();
        
        // CROCS hash
        uint64_t h = 0;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
        for (size_t j = 0; j < 8; j++) {
            h = h * prime + bytes[j];
            h ^= h >> 32;
        }
        h = (h * prime) % table_size;
        
        sampled_buckets[h]++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    std::cout << "Performance: " << (double)ns / sample_size << " ns/hash\n";
    std::cout << "Unique hashes in sample: " << sampled_buckets.size() << "/" << sample_size << "\n";
    
    // Collision analysis
    size_t collisions = sample_size - sampled_buckets.size();
    double collision_rate = (double)collisions / sample_size;
    
    // Expected collisions for uniform distribution
    double expected_collisions = sample_size * (1.0 - exp(-1.0 * sample_size / table_size));
    if (table_size < sample_size) {
        expected_collisions = sample_size - table_size;
    }
    
    std::cout << "Collisions: " << collisions << " (rate: " << collision_rate * 100 << "%)\n";
    std::cout << "Expected: " << expected_collisions << "\n";
    std::cout << "Ratio: " << (collisions / expected_collisions) << "\n";
}

// Cryptographic dual-table attack demonstration
void dual_table_attack_demo() {
    std::cout << "\n=== DUAL TABLE CRYPTOGRAPHIC ATTACK ===\n";
    
    // Use 48-bit secret for demo (still huge: ~281 trillion)
    uint64_t secret = 123456789012345ULL;
    uint64_t max_48bit = (1ULL << 48) - 1;
    uint64_t complement = max_48bit - secret;
    
    std::cout << "Secret S: " << secret << "\n";
    std::cout << "Table N1: " << secret << "\n";
    std::cout << "Table N2: " << complement << " (2^48 - S)\n";
    
    uint64_t p1 = GoldenPrimeFinder::find_golden_prime(secret);
    uint64_t p2 = GoldenPrimeFinder::find_golden_prime(complement);
    
    std::cout << "Prime P1: " << p1 << "\n";
    std::cout << "Prime P2: " << p2 << "\n";
    std::cout << "P1 + P2: " << p1 + p2 << "\n";
    std::cout << "2^48/φ: " << (uint64_t)(max_48bit / PHI) << "\n";
    std::cout << "Difference: " << (int64_t)(p1 + p2) - (int64_t)(max_48bit / PHI) << "\n";
    
    // Show the algebraic relationship
    std::cout << "\nAlgebraic attack:\n";
    std::cout << "If attacker knows P1 + P2 ≈ 2^48/φ\n";
    std::cout << "And observes collision patterns...\n";
    std::cout << "They can narrow down S significantly!\n";
}

int main(int argc, char* argv[]) {
    std::cout << "CROCS Huge Table Testing\n";
    std::cout << "========================\n";
    
    if (argc > 1 && std::string(argv[1]) == "--attack") {
        dual_table_attack_demo();
        return 0;
    }
    
    // Test increasing powers of 2
    std::vector<uint64_t> sizes = {
        (1ULL << 20) - 1,  // ~1M
        (1ULL << 30) - 1,  // ~1B  
        (1ULL << 40) - 1,  // ~1T
        (1ULL << 48) - 1,  // ~281T
        (1ULL << 56) - 1,  // ~72 quadrillion
        (1ULL << 60) - 1,  // ~1 quintillion
        (1ULL << 63) - 1   // ~9 quintillion (half of 64-bit)
    };
    
    for (uint64_t size : sizes) {
        test_huge_table(size);
    }
    
    std::cout << "\n=== CRYPTOGRAPHIC IMPLICATIONS ===\n";
    std::cout << "1. Golden ratio prime selection scales to huge tables\n";
    std::cout << "2. BUT the algebraic structure (P ≈ N/φ) is exploitable\n";
    std::cout << "3. For cryptographic use, need to break this relationship\n";
    std::cout << "4. Possible solutions:\n";
    std::cout << "   - Use P = nearest_prime(N/φ + random_offset)\n";
    std::cout << "   - Multiple rounds with different primes\n";
    std::cout << "   - Combine with proven cryptographic primitives\n";
    
    return 0;
}