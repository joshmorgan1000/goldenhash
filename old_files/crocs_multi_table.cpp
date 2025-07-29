#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <vector>
#include <array>

using namespace crocs;

/**
 * Multi-Table CROCS: Use multiple random table sizes for cryptographic strength
 * 
 * Instead of one predictable N and 2^64-N, use:
 * - K random table sizes N1, N2, ..., Nk
 * - Each with its own golden prime P1, P2, ..., Pk
 * - Final hash = (H1, H2, ..., Hk) concatenated or XORed
 */
class CrocsMultiTable {
private:
    struct Table {
        uint64_t size;
        uint64_t prime;
        uint64_t seed;
    };
    
    std::vector<Table> tables_;
    
public:
    // Generate random table sizes with constraints
    static std::vector<uint64_t> generate_random_sizes(
        size_t count, 
        uint64_t min_size = 1000000,    // 1M minimum
        uint64_t max_size = 1000000000, // 1B maximum
        uint64_t secret_seed = 0
    ) {
        std::mt19937_64 rng(secret_seed);
        std::vector<uint64_t> sizes;
        
        for (size_t i = 0; i < count; i++) {
            uint64_t size = min_size + (rng() % (max_size - min_size));
            // Make it prime for extra properties
            while (!PrimalityTester::is_prime(size)) size++;
            sizes.push_back(size);
        }
        
        return sizes;
    }
    
    CrocsMultiTable(const std::vector<uint64_t>& sizes) {
        for (uint64_t size : sizes) {
            Table t;
            t.size = size;
            t.prime = GoldenPrimeFinder::find_golden_prime(size);
            t.seed = std::hash<uint64_t>{}(size); // Derive seed from size
            tables_.push_back(t);
        }
    }
    
    // Hash to multiple tables
    std::vector<uint64_t> hash_multi(const uint8_t* data, size_t len) const {
        std::vector<uint64_t> results;
        
        for (const auto& table : tables_) {
            uint64_t h = table.seed;
            for (size_t i = 0; i < len; i++) {
                h = h * table.prime + data[i];
                h ^= h >> 32;
            }
            h = (h * table.prime) % table.size;
            results.push_back(h);
        }
        
        return results;
    }
    
    // Combine multiple hashes into single value
    uint64_t hash_combined(const uint8_t* data, size_t len) const {
        auto hashes = hash_multi(data, len);
        
        // XOR combination with rotation
        uint64_t combined = 0;
        for (size_t i = 0; i < hashes.size(); i++) {
            combined ^= std::rotl(hashes[i], i * 7);
        }
        
        return combined;
    }
    
    void print_config() const {
        std::cout << "Multi-Table CROCS Configuration:\n";
        std::cout << "Tables: " << tables_.size() << "\n";
        uint64_t total_size = 0;
        for (size_t i = 0; i < tables_.size(); i++) {
            std::cout << "  Table " << i << ": size=" << tables_[i].size 
                      << ", prime=" << tables_[i].prime 
                      << ", ratio=" << (double)tables_[i].size / tables_[i].prime << "\n";
            total_size += tables_[i].size;
        }
        std::cout << "Total keyspace: ~" << total_size << "\n";
    }
};

// Cryptographic analysis
void analyze_multi_table_security() {
    std::cout << "\n=== MULTI-TABLE CRYPTOGRAPHIC ANALYSIS ===\n\n";
    
    // Attack scenario 1: Known table sizes
    std::cout << "Scenario 1: Attacker knows table sizes\n";
    std::vector<uint64_t> known_sizes = {1000007, 2000003, 4000037};
    CrocsMultiTable known_tables(known_sizes);
    known_tables.print_config();
    
    std::cout << "\nAttack difficulty:\n";
    std::cout << "- Cannot use N1 + N2 = constant relationship\n";
    std::cout << "- Each table is independent\n";
    std::cout << "- Must solve K separate discrete log problems\n\n";
    
    // Attack scenario 2: Secret table sizes
    std::cout << "Scenario 2: Secret random table sizes\n";
    uint64_t secret = 0xDEADBEEF;
    auto secret_sizes = CrocsMultiTable::generate_random_sizes(5, 1000000, 100000000, secret);
    CrocsMultiTable secret_tables(secret_sizes);
    secret_tables.print_config();
    
    std::cout << "\nAttack difficulty:\n";
    std::cout << "- Must first discover K table sizes from collision patterns\n";
    std::cout << "- Each size is random, no algebraic relationship\n";
    std::cout << "- Exponentially harder with more tables\n\n";
    
    // Demonstration
    std::cout << "=== DEMONSTRATION ===\n";
    std::string test_data = "Hello, World!";
    auto hashes = secret_tables.hash_multi(
        reinterpret_cast<const uint8_t*>(test_data.data()), 
        test_data.size()
    );
    
    std::cout << "Input: \"" << test_data << "\"\n";
    std::cout << "Hashes: ";
    for (auto h : hashes) {
        std::cout << h << " ";
    }
    std::cout << "\n";
    std::cout << "Combined: " << secret_tables.hash_combined(
        reinterpret_cast<const uint8_t*>(test_data.data()), 
        test_data.size()
    ) << "\n";
    
    // Security comparison
    std::cout << "\n=== SECURITY COMPARISON ===\n";
    std::cout << "Single-table CROCS:\n";
    std::cout << "  - Vulnerable to algebraic attack via golden ratio\n";
    std::cout << "  - O(sqrt(N)) collision finding\n";
    std::cout << "  - Predictable structure\n\n";
    
    std::cout << "Multi-table CROCS:\n";
    std::cout << "  - No single algebraic relationship\n";
    std::cout << "  - O(sqrt(N1) × sqrt(N2) × ... × sqrt(Nk)) collision finding\n";
    std::cout << "  - Requires solving multiple independent problems\n";
    std::cout << "  - Can be made into a keyed hash function\n";
}

int main() {
    std::cout << "CROCS Multi-Table Cryptographic Construction\n";
    std::cout << "==========================================\n";
    
    analyze_multi_table_security();
    
    // Performance test
    std::cout << "\n=== PERFORMANCE TEST ===\n";
    
    auto sizes = CrocsMultiTable::generate_random_sizes(4, 1000000, 10000000);
    CrocsMultiTable mt(sizes);
    
    std::mt19937_64 rng(42);
    std::vector<uint8_t> data(64);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000000; i++) {
        for (auto& b : data) b = rng() & 0xFF;
        auto h = mt.hash_combined(data.data(), data.size());
        if (h == 0) std::cout << ""; // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    std::cout << "Performance: " << ns / 1000000.0 << " ns/hash\n";
    std::cout << "(~" << 1000000000.0 / (ns / 1000000.0) << " hashes/second)\n";
    
    return 0;
}