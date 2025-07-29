/**
 * @file golden_hash_json.cpp
 * @brief Simple golden ratio hash with JSON output for Python analysis
 */

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <cstring>
#include <bit>

// Golden ratio
constexpr double PHI = 1.6180339887498948482;

// Simple primality test
bool is_prime(uint64_t n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (uint64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// Find nearest prime to target
uint64_t find_nearest_prime(uint64_t target, uint64_t max_value) {
    if (target > max_value) target = max_value;
    
    if (is_prime(target)) return target;
    
    for (uint64_t delta = 1; delta < target; ++delta) {
        if (target > delta && is_prime(target - delta)) {
            return target - delta;
        }
        if (target + delta <= max_value && is_prime(target + delta)) {
            return target + delta;
        }
    }
    return 2;
}

// Simple factorization
std::vector<uint64_t> factorize(uint64_t n) {
    std::vector<uint64_t> factors;
    uint64_t temp = n;
    
    for (uint64_t i = 2; i * i <= temp; i++) {
        while (temp % i == 0) {
            factors.push_back(i);
            temp /= i;
        }
    }
    if (temp > 1) factors.push_back(temp);
    
    return factors;
}

// Simple golden ratio hash using XOR mixing
class GoldenHashN {
private:
    uint64_t n_;        // Number of buckets
    uint64_t prime_;    // Golden ratio prime
    int bits_;          // Number of bits needed
    
public:
    GoldenHashN(uint64_t n) : n_(n) {
        // Calculate bits needed
        bits_ = 0;
        uint64_t temp = n - 1;
        while (temp > 0) {
            bits_++;
            temp >>= 1;
        }
        
        // Calculate golden ratio prime
        uint64_t golden_value = static_cast<uint64_t>(n / PHI);
        prime_ = find_nearest_prime(golden_value, n - 1);
    }
    
    uint64_t hash(const uint8_t* data, size_t len) const {
        uint64_t h = 0;
        for (size_t i = 0; i < len; i++) {
            h = h * prime_ + data[i];
            // Add mixing step to avoid patterns
            h ^= h >> (bits_ / 2);
        }
        // Final mixing
        h *= prime_;
        h ^= h >> (bits_ - bits_ / 3);
        return h % n_;
    }
    
    uint64_t get_prime() const { return prime_; }
    int get_bits() const { return bits_; }
    uint64_t get_n() const { return n_; }
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <table_size> <num_tests> [--json]\n";
        return 1;
    }
    
    uint64_t table_size = std::stoull(argv[1]);
    size_t num_tests = std::stoull(argv[2]);
    bool json_output = (argc == 4 && std::string(argv[3]) == "--json");
    
    // Create hash function
    GoldenHashN hasher(table_size);
    
    if (!json_output) {
        std::cout << "Golden Ratio Hash Test\n";
        std::cout << "=====================\n\n";
        std::cout << "Table size (N): " << table_size << "\n";
        std::cout << "Is prime: " << (is_prime(table_size) ? "Yes" : "No") << "\n";
        std::cout << "Bits needed: " << hasher.get_bits() << "\n";
        std::cout << "Golden value: " << uint64_t(table_size/PHI) << "\n";
        std::cout << "Selected prime: " << hasher.get_prime() << "\n";
    }
    
    // Generate test data
    std::mt19937_64 rng(42);
    std::vector<std::vector<uint8_t>> test_data;
    for (size_t i = 0; i < num_tests; i++) {
        std::vector<uint8_t> data(16 + (i % 48)); // Vary size 16-64 bytes
        for (auto& byte : data) {
            byte = rng() & 0xFF;
        }
        test_data.push_back(data);
    }
    
    // Hash and collect statistics
    std::vector<uint64_t> hash_counts(table_size, 0);
    size_t total_bit_changes = 0;
    size_t total_bit_tests = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < test_data.size(); i++) {
        const auto& data = test_data[i];
        uint64_t h = hasher.hash(data.data(), data.size());
        hash_counts[h]++;
        
        // Test avalanche for subset
        if (i % 100 == 0) {
            for (size_t byte_idx = 0; byte_idx < data.size() && byte_idx < 32; byte_idx++) {
                for (int bit = 0; bit < 8; bit++) {
                    std::vector<uint8_t> modified = data;
                    modified[byte_idx] ^= (1 << bit);
                    
                    uint64_t h2 = hasher.hash(modified.data(), modified.size());
                    uint64_t diff = h ^ h2;
                    
                    // Count bits in the range we actually use
                    int bits_to_count = hasher.get_bits();
                    uint64_t mask = (bits_to_count >= 64) ? ~0ULL : ((1ULL << bits_to_count) - 1);
                    total_bit_changes += __builtin_popcountll(diff & mask);
                    total_bit_tests++;
                }
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate statistics
    uint64_t unique_hashes = 0;
    uint64_t max_collisions = 0;
    double expected = double(num_tests) / table_size;
    double chi_square = 0.0;
    
    for (uint64_t count : hash_counts) {
        if (count > 0) unique_hashes++;
        if (count > max_collisions) max_collisions = count;
        
        double diff = count - expected;
        chi_square += (diff * diff) / expected;
    }
    chi_square /= table_size;
    
    uint64_t total_collisions = num_tests - unique_hashes;
    
    // Calculate expected collisions
    double expected_unique = table_size * (1.0 - std::exp(-double(num_tests) / table_size));
    double expected_collisions = num_tests - expected_unique;
    double collision_ratio = (expected_collisions > 0) ? total_collisions / expected_collisions : 1.0;
    
    // Calculate avalanche score
    double avalanche_score = (total_bit_tests > 0) ? 
        double(total_bit_changes) / (total_bit_tests * hasher.get_bits()) : 0.0;
    
    // Test vectors
    struct TestVector {
        const char* str;
        const char* name;
    };
    
    TestVector vectors[] = {
        {"", "empty"},
        {"a", "a"},
        {"abc", "abc"},
        {"message digest", "message_digest"},
        {"abcdefghijklmnopqrstuvwxyz", "alphabet"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "alphanumeric"},
        {"The quick brown fox jumps over the lazy dog", "fox"}
    };
    
    std::vector<std::pair<std::string, uint64_t>> test_hashes;
    for (const auto& vec : vectors) {
        uint64_t h = hasher.hash(reinterpret_cast<const uint8_t*>(vec.str), strlen(vec.str));
        test_hashes.push_back({vec.name, h});
    }
    
    // Get factors
    auto factors = factorize(table_size);
    std::string factors_str;
    for (size_t i = 0; i < factors.size(); i++) {
        factors_str += std::to_string(factors[i]);
        if (i < factors.size() - 1) factors_str += ", ";
    }
    
    if (!json_output) {
        // Human readable output
        std::cout << "\nResults:\n";
        std::cout << "--------\n";
        std::cout << "Total time: " << duration.count() / 1000.0 << " ms\n";
        std::cout << "Performance: " << duration.count() * 1000.0 / num_tests << " ns/hash\n";
        std::cout << "Unique hashes: " << unique_hashes << "/" << num_tests << "\n";
        std::cout << "Total collisions: " << total_collisions << "\n";
        std::cout << "Expected collisions: " << expected_collisions << "\n";
        std::cout << "Collision ratio: " << collision_ratio << " (ideal: 1.0)\n";
        std::cout << "Max bucket load: " << max_collisions << "\n";
        std::cout << "Chi-square: " << chi_square << " (ideal: 1.0)\n";
        std::cout << "Avalanche score: " << avalanche_score << " (ideal: 0.5)\n";
        
        std::cout << "\nTest vectors:\n";
        for (const auto& [name, hash] : test_hashes) {
            std::cout << "  H(\"" << name << "\"): " << hash << "\n";
        }
    } else {
        // JSON output
        std::cout << "{\n";
        std::cout << "  \"table_size\": " << table_size << ",\n";
        std::cout << "  \"unique_hashes\": " << unique_hashes << ",\n";
        std::cout << "  \"distribution_uniformity\": " << std::sqrt(chi_square / table_size) << ",\n";
        std::cout << "  \"total_collisions\": " << total_collisions << ",\n";
        std::cout << "  \"expected_collisions\": " << expected_collisions << ",\n";
        std::cout << "  \"collision_ratio\": " << collision_ratio << ",\n";
        std::cout << "  \"max_bucket_load\": " << max_collisions << ",\n";
        std::cout << "  \"avalanche_score\": " << avalanche_score << ",\n";
        std::cout << "  \"chi_square\": " << chi_square << ",\n";
        std::cout << "  \"prime_high\": " << hasher.get_prime() << ",\n";
        std::cout << "  \"prime_low\": 0,\n";  // Not used in this version
        std::cout << "  \"working_modulus\": " << table_size << ",\n";
        std::cout << "  \"test_vectors\": {\n";
        for (size_t i = 0; i < test_hashes.size(); i++) {
            std::cout << "    \"" << test_hashes[i].first << "\": " << test_hashes[i].second;
            if (i < test_hashes.size() - 1) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  },\n";
        std::cout << "  \"factors\": \"" << factors_str << "\",\n";
        std::cout << "  \"performance_ns_per_hash\": " 
                  << (duration.count() * 1000.0 / num_tests) << "\n";
        std::cout << "}\n";
    }
    
    return 0;
}