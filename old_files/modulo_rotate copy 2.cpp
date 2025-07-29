#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <cstring>
#include <algorithm>

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

// Find factors of N for mixed-radix representation
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

// Modular hash function using golden ratio and modular "rotation"
class ModularGoldenHash {
private:
    uint64_t N;              // Table size
    uint64_t prime_high;     // Prime near N/φ
    uint64_t prime_low;      // Prime near N/φ²
    uint64_t working_mod;    // Modulus for operations
    std::vector<uint64_t> factors;
    std::vector<uint64_t> secret;  // Modular secret for mixing
    
public:
    ModularGoldenHash(uint64_t table_size) : N(table_size) {
        // Find golden ratio primes
        uint64_t target_high = N / PHI;
        uint64_t target_low = N / (PHI * PHI);
        
        // Find nearest primes
        prime_high = find_nearest_prime(target_high);
        prime_low = find_nearest_prime(target_low);
        
        // Determine working modulus
        if (is_prime(N)) {
            // For prime N, we have options:
            // 1. Use N-1 (Fermat's little theorem) - causes issues with consecutive primes
            // 2. Use next composite number
            // 3. Use N+1 
            // For now, use N+1 to avoid collision issues
            working_mod = N + 1;
        } else {
            // For composite N, use N
            working_mod = N;
        }
        
        // Factorize for mixed-radix if needed
        factors = factorize(working_mod);
        
        // Initialize secret for 3 complete rotations
        // For modular arithmetic, we want values that cycle through the space
        size_t secret_size = 24;  // Keep same structure as XXH3
        secret.resize(secret_size);
        
        // Generate secret values using golden ratio mixing
        uint64_t h = N;
        for (size_t i = 0; i < secret_size; i++) {
            h = h * prime_high + i;
            h = (h + h / 33) % working_mod;
            h = (h * prime_low) % working_mod;
            h = (h + h / 29) % working_mod;
            // Already in modular space
            secret[i] = h;
        }
    }
    
    uint64_t hash(const uint8_t* data, size_t len) const {
        uint64_t h = 0;
        
        // Process each byte with secret mixing
        for (size_t i = 0; i < len; i++) {
            // Mix with secret value (3 rotations through 24 values)
            uint64_t secret_val = secret[i % secret.size()];
            
            // Input mixing with secret (no XOR - use addition)
            h = (h + data[i] + secret_val) % working_mod;
            h = (h * prime_low) % working_mod;
            
            // Modular "rotation" - multiply by position-dependent value
            // This simulates rotation in modular space
            h = (h * (prime_high + i * secret_val)) % working_mod;
        }
        
        // Final avalanche mixing in modular space
        // Equivalent to xxh3_avalanche but in base(working_mod)
        // h ^= h >> 37 becomes h = (h + h/divisor1) % working_mod
        // where divisor1 ≈ working_mod^(37/64)
        uint64_t divisor1 = std::max<uint64_t>(2, working_mod / 3);  // ~1/3 of space
        h = (h + (h / divisor1)) % working_mod;
        
        // Multiply by our version of the avalanche prime
        // In XXH3: 0x165667919E3779F9 which is related to golden ratio
        // For us: use a prime near working_mod/φ³
        uint64_t avalanche_prime = find_nearest_prime(working_mod / (PHI * PHI * PHI));
        h = (h * avalanche_prime) % working_mod;
        
        // h ^= h >> 32 becomes h = (h + h/divisor2) % working_mod  
        // where divisor2 ≈ working_mod^(32/64) = sqrt(working_mod)
        uint64_t divisor2 = std::max<uint64_t>(2, (uint64_t)std::sqrt(working_mod));
        h = (h + (h / divisor2)) % working_mod;
        
        // Final reduction to table size
        if (working_mod != N) {
            h = h % N;
        }
        
        return h;
    }
    
    void print_info() const {
        std::cout << "Table size (N): " << N << "\n";
        std::cout << "Is prime: " << (is_prime(N) ? "Yes" : "No") << "\n";
        std::cout << "Working modulus: " << working_mod << "\n";
        std::cout << "High prime (N/φ): " << prime_high << " (target: " << uint64_t(N/PHI) << ")\n";
        std::cout << "Low prime (N/φ²): " << prime_low << " (target: " << uint64_t(N/(PHI*PHI)) << ")\n";
        std::cout << "Factorization: ";
        for (auto f : factors) std::cout << f << " ";
        std::cout << "\n";
        std::cout << "Golden ratio check: N/prime_high = " << double(N)/prime_high << " (φ = " << PHI << ")\n";
    }
    
    uint64_t get_table_size() const { return N; }
    uint64_t get_prime_high() const { return prime_high; }
    uint64_t get_prime_low() const { return prime_low; }
    uint64_t get_working_mod() const { return working_mod; }
    const std::vector<uint64_t>& get_factors() const { return factors; }
    
    
private:
    uint64_t find_nearest_prime(uint64_t target) const {
        // Search outward from target
        for (uint64_t delta = 0; delta < 1000; delta++) {
            if (target > delta && is_prime(target - delta)) return target - delta;
            if (is_prime(target + delta)) return target + delta;
        }
        return target; // Fallback
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <table_size> <num_tests> [--json]\n";
        std::cerr << "Example: " << argv[0] << " 5829235 24000\n";
        std::cerr << "Example: " << argv[0] << " 5829235 24000 --json\n";
        return 1;
    }
    
    uint64_t table_size = std::stoull(argv[1]);
    size_t num_tests = std::stoull(argv[2]);
    bool json_output = (argc == 4 && std::string(argv[3]) == "--json");
    
    // Create hash function
    ModularGoldenHash hasher(table_size);
    
    if (!json_output) {
        std::cout << "Modular Golden Ratio Hash Test\n";
        std::cout << "==============================\n\n";
        hasher.print_info();
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
    
    if (!json_output) {
        std::cout << "\nRunning " << num_tests << " hash operations...\n";
    }
    
    // Hash and collect statistics, including avalanche
    std::vector<uint64_t> hash_counts(table_size, 0);
    size_t total_bit_changes = 0;
    size_t total_bit_tests = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < test_data.size(); i++) {
        const auto& data = test_data[i];
        uint64_t h = hasher.hash(data.data(), data.size());
        hash_counts[h]++;
        
        // Test avalanche for a subset of inputs (every 100th) to keep performance reasonable
        if (i % 100 == 0) {
            // Test single bit flips
            for (size_t byte_idx = 0; byte_idx < data.size() && byte_idx < 32; byte_idx++) {
                for (int bit = 0; bit < 8; bit++) {
                    // Make a copy and flip bit
                    std::vector<uint8_t> modified = data;
                    modified[byte_idx] ^= (1 << bit);
                    
                    // Hash modified
                    uint64_t h2 = hasher.hash(modified.data(), modified.size());
                    
                    // Count differing bits (only bits that matter for table_size)
                    uint64_t diff = h ^ h2;
                    // For small table sizes, only count relevant bits
                    int bits_to_count = 64;
                    if (table_size < (1ULL << 32)) {
                        bits_to_count = 32 - __builtin_clz(table_size - 1) + 1;
                    }
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
    
    // Calculate expected collisions (birthday paradox)
    double expected_unique = table_size * (1.0 - std::exp(-double(num_tests) / table_size));
    double expected_collisions = num_tests - expected_unique;
    double collision_ratio = (expected_collisions > 0) ? total_collisions / expected_collisions : 1.0;
    
    // Calculate avalanche score
    // For modular hashing, we need to account for the limited output space
    int output_bits = (table_size < 2) ? 1 : (64 - __builtin_clzll(table_size - 1));
    double avalanche_score = (total_bit_tests > 0) ? 
        double(total_bit_changes) / (total_bit_tests * output_bits) : 0.0;
    
    // Hash standard test vectors
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
    
    if (!json_output) {
        // Output results
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
        
        // Show test vectors
        std::cout << "\nTest vectors:\n";
        for (const auto& [name, hash] : test_hashes) {
            std::cout << "  H(\"" << name << "\"): " << hash << "\n";
        }
        
        // Predecessor function check (N/φ)
        std::cout << "\nPredecessor function check:\n";
        std::cout << "N / φ = " << table_size << " / " << PHI << " = " << table_size / PHI << "\n";
        std::cout << "φ * " << uint64_t(table_size / PHI) << " = " << PHI * uint64_t(table_size / PHI) << "\n";
        std::cout << "Difference from N: " << int64_t(table_size) - int64_t(PHI * uint64_t(table_size / PHI)) << "\n";
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
        std::cout << "  \"prime_high\": " << hasher.get_prime_high() << ",\n";
        std::cout << "  \"prime_low\": " << hasher.get_prime_low() << ",\n";
        std::cout << "  \"working_modulus\": " << hasher.get_working_mod() << ",\n";
        std::cout << "  \"test_vectors\": {\n";
        for (size_t i = 0; i < test_hashes.size(); i++) {
            std::cout << "    \"" << test_hashes[i].first << "\": " << test_hashes[i].second;
            if (i < test_hashes.size() - 1) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  },\n";
        std::cout << "  \"factors\": \"";
        for (size_t i = 0; i < hasher.get_factors().size(); i++) {
            std::cout << hasher.get_factors()[i];
            if (i < hasher.get_factors().size() - 1) std::cout << ", ";
        }
        std::cout << "\",\n";
        std::cout << "  \"performance_ns_per_hash\": " 
                  << (duration.count() * 1000.0 / num_tests) << "\n";
        std::cout << "}\n";
    }
    
    return 0;
}