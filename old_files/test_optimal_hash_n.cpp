/**
 * @file test_optimal_hash_n.cpp
 * @brief Create optimal hash functions for N possible values using golden ratio
 */

#include <psyfer.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <random>
#include <iomanip>
#include <chrono>

// Golden ratio
constexpr double PHI = 1.6180339887498948482;

// Improved primality test
bool is_prime(uint64_t n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (uint64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// Find nearest prime to target within range
uint64_t find_nearest_prime(uint64_t target, uint64_t max_value) {
    if (target > max_value) target = max_value;
    
    // Check target first
    if (is_prime(target)) return target;
    
    // Search outward
    for (uint64_t delta = 1; delta < target; ++delta) {
        if (target > delta && is_prime(target - delta)) {
            return target - delta;
        }
        if (target + delta <= max_value && is_prime(target + delta)) {
            return target + delta;
        }
    }
    return 2; // Fallback to smallest prime
}

// Simple hash function using golden ratio prime
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
    
    uint64_t hash(const std::string& key) const {
        uint64_t h = 0;
        for (char c : key) {
            h = h * prime_ + static_cast<uint8_t>(c);
            // Add mixing step to avoid patterns with sequential inputs
            h ^= h >> (bits_ / 2);
        }
        // Final mixing - shift by approximately bits_/2 to mix high bits into low
        h *= prime_;
        h ^= h >> (bits_ - bits_ / 3);
        return h % n_;
    }
    
    uint64_t hash_bytes(const std::vector<std::byte>& data) const {
        uint64_t h = 0;
        for (std::byte b : data) {
            h = h * prime_ + static_cast<uint8_t>(b);
            // Add mixing step to avoid patterns with sequential inputs
            h ^= h >> (bits_ / 2);
        }
        // Final mixing - shift by approximately bits_/2 to mix high bits into low
        h *= prime_;
        h ^= h >> (bits_ - bits_ / 3);
        return h % n_;
    }
    
    uint64_t get_prime() const { return prime_; }
    int get_bits() const { return bits_; }
};

// Test collision rate
void test_hash_quality(uint64_t n, int num_tests) {
    GoldenHashN hasher(n);
    
    std::log_info("\n=== Testing hash function for N={} ===", n);
    std::log_info("Bits needed: {}", hasher.get_bits());
    std::log_info("Golden value: {} (0x{:X})", static_cast<uint64_t>(n / PHI), 
                  static_cast<uint64_t>(n / PHI));
    std::log_info("Selected prime: {} (0x{:X})", hasher.get_prime(), hasher.get_prime());
    
    // Only run collision tests if they make sense
    bool skip_collision_tests = (n < 100 && static_cast<uint64_t>(num_tests) > n * 2);
    
    if (!skip_collision_tests) {
    
    // Test sequential keys
    std::unordered_map<uint64_t, std::vector<std::string>> collisions;
    for (int i = 0; i < num_tests; ++i) {
        std::string key = "test" + std::to_string(i);
        uint64_t h = hasher.hash(key);
        collisions[h].push_back(key);
    }
    
    // Analyze collisions
    int total_collisions = 0;
    int max_collisions = 0;
    for (const auto& [hash, keys] : collisions) {
        if (keys.size() > 1) {
            total_collisions += keys.size() - 1;
            max_collisions = std::max(max_collisions, static_cast<int>(keys.size()));
        }
    }
    
    // Using birthday paradox: expected number of items that collide (not pairs)
    // Approximation: num_tests - n * (1 - exp(-num_tests/n))
    double expected_unique = n * (1.0 - std::exp(-static_cast<double>(num_tests) / n));
    double expected_collisions = num_tests - expected_unique;
    double load_factor = static_cast<double>(num_tests) / n;
    
    std::log_info("\nSequential test results:");
    std::log_info("  Tests: {}, Unique hashes: {}", num_tests, collisions.size());
    std::log_info("  Total collisions: {}", total_collisions);
    std::log_info("  Max bucket size: {}", max_collisions);
    std::log_info("  Load factor: {:.2f}", load_factor);
    std::log_info("  Expected collisions (birthday): {:.2f}", expected_collisions);
    std::log_info("  Actual/Expected ratio: {:.2f}", 
                  expected_collisions > 0 ? total_collisions / expected_collisions : 0);
    
    // Test random keys
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1000000);
    collisions.clear();
    
    for (int i = 0; i < num_tests; ++i) {
        std::string key = "random" + std::to_string(dist(rng));
        uint64_t h = hasher.hash(key);
        collisions[h].push_back(key);
    }
    
    // Analyze random collisions
    total_collisions = 0;
    for (const auto& [hash, keys] : collisions) {
        if (keys.size() > 1) {
            total_collisions += keys.size() - 1;
        }
    }
    
    std::log_info("\nRandom test results:");
    std::log_info("  Unique hashes: {}", collisions.size());
    std::log_info("  Total collisions: {}", total_collisions);
    
    } else {
        std::log_info("\nSkipping collision tests for small N={} (collisions guaranteed)", n);
    }
    
    // Test distribution uniformity
    std::vector<int> bucket_counts(n, 0);
    for (int i = 0; i < n * 10; ++i) {
        std::string key = "dist" + std::to_string(i);
        uint64_t h = hasher.hash(key);
        bucket_counts[h]++;
    }
    
    // Calculate chi-square statistic
    double expected_per_bucket = 10.0;
    double chi_square = 0;
    for (int count : bucket_counts) {
        double diff = count - expected_per_bucket;
        chi_square += (diff * diff) / expected_per_bucket;
    }
    
    std::log_info("\nDistribution test:");
    std::log_info("  Chi-square: {:.2f} (lower is better)", chi_square);
    std::log_info("  Expected for uniform: ~{}", n);
}

// Dynamic hash function builder
class DynamicHashN {
private:
    uint64_t n_;
    std::function<uint64_t(uint64_t, uint64_t)> mixer_;
    std::function<uint64_t(uint64_t)> finalizer_;
    uint64_t multiplier_;
    
public:
    DynamicHashN(uint64_t n, 
                 uint64_t multiplier,
                 std::function<uint64_t(uint64_t, uint64_t)> mixer = nullptr,
                 std::function<uint64_t(uint64_t)> finalizer = nullptr) 
        : n_(n), multiplier_(multiplier) {
        
        // Default mixer: simple XOR shift
        if (!mixer) {
            mixer_ = [](uint64_t h, uint64_t bits) {
                return h ^ (h >> (bits / 2));
            };
        } else {
            mixer_ = mixer;
        }
        
        // Default finalizer: multiply and shift
        if (!finalizer) {
            finalizer_ = [this](uint64_t h) {
                h *= multiplier_;
                h ^= h >> 27;
                return h;
            };
        } else {
            finalizer_ = finalizer;
        }
    }
    
    uint64_t hash(const std::string& key) const {
        uint64_t h = 0;
        int bits = 64 - __builtin_clzll(n_ - 1);
        
        for (char c : key) {
            h = h * multiplier_ + static_cast<uint8_t>(c);
            h = mixer_(h, bits);
        }
        
        return finalizer_(h) % n_;
    }
};

// Fibonacci hash comparison
class FibonacciHashN {
private:
    uint64_t n_;
    uint64_t multiplier_;
    
public:
    FibonacciHashN(uint64_t n) : n_(n) {
        // Use Fibonacci hashing constant (2^64 / φ)
        multiplier_ = 11400714819323198485ULL; // 2^64 / φ
    }
    
    uint64_t hash(const std::string& key) const {
        uint64_t h = 0;
        for (char c : key) {
            h = h * 31 + static_cast<uint8_t>(c); // Standard string hash
        }
        return (h * multiplier_) % n_;
    }
};

int main() {
    std::log_info("=== Optimal Hash Functions for N Values ===");
    std::log_info("Using golden ratio formula: Prime ≈ N / φ");
    
    // Test various N values from 1000 to 32-bit
    std::vector<uint64_t> test_sizes = {
        1009,       // Prime near 1000
        10007,      // Prime near 10000
        100003,     // Prime near 100000
        1000003,    // Prime near 1 million
        1024, 16384, 65536, 1048576,   // Powers of 2
        10000019,   // Prime near 10 million
        100000007,  // Prime near 100 million
        16777216,   // 2^24
        268435456,  // 2^28
        1000000007, // Prime near 1 billion
        2147483647, // 2^31-1 (max 32-bit signed, also prime!)
        4294967291  // Largest 32-bit prime
    };
    
    for (uint64_t n : test_sizes) {
        // Use more tests for smaller N values, fewer for very large N
        uint64_t num_tests;
        if (n <= 10000) {
            num_tests = n * 10;  // High load factor for small tables
        } else if (n <= 1000000) {
            num_tests = 100000;  // Fixed 100k tests for medium tables
        } else {
            num_tests = 1000000; // 1M tests for large tables
        }
        test_hash_quality(n, num_tests);
    }
    
    // Test dynamic hash builder
    std::log_info("\n\n=== Testing Dynamic Hash Builder ===");
    {
        uint64_t n = 10007;
        uint64_t golden_prime = find_nearest_prime(static_cast<uint64_t>(n / PHI), n - 1);
        
        // Create different hash functions with the same multiplier
        DynamicHashN hash1(n, golden_prime); // Default mixer and finalizer
        
        DynamicHashN hash2(n, golden_prime, 
            // Custom mixer: rotate instead of shift
            [](uint64_t h, uint64_t bits) {
                return std::rotl(h, bits / 3);
            }
        );
        
        DynamicHashN hash3(n, golden_prime,
            nullptr, // default mixer
            // Custom finalizer: more aggressive mixing
            [golden_prime](uint64_t h) {
                h ^= h >> 33;
                h *= golden_prime;
                h ^= h >> 29;
                h *= 0x165667919E3779F9ULL;
                h ^= h >> 32;
                return h;
            }
        );
        
        // Test them
        std::unordered_map<uint64_t, int> counts1, counts2, counts3;
        
        for (int i = 0; i < 50000; ++i) {
            std::string key = "test" + std::to_string(i);
            counts1[hash1.hash(key)]++;
            counts2[hash2.hash(key)]++;
            counts3[hash3.hash(key)]++;
        }
        
        std::log_info("Dynamic hash results for N={}:", n);
        std::log_info("  Default mixer/finalizer: {} unique values", counts1.size());
        std::log_info("  Custom mixer (rotate):   {} unique values", counts2.size());
        std::log_info("  Custom finalizer:        {} unique values", counts3.size());
    }
    
    // Compare with Fibonacci hashing
    std::log_info("\n\n=== Comparison with Fibonacci Hashing ===");
    
    uint64_t n = 1000;
    int num_tests = 10000;
    
    // Test golden ratio hash
    {
        GoldenHashN hasher(n);
        std::unordered_map<uint64_t, int> counts;
        for (int i = 0; i < num_tests; ++i) {
            std::string key = "test" + std::to_string(i);
            counts[hasher.hash(key)]++;
        }
        
        int collisions = 0;
        for (const auto& [h, count] : counts) {
            if (count > 1) collisions += count - 1;
        }
        
        std::log_info("Golden ratio hash (N={}): {} collisions", n, collisions);
    }
    
    // Test Fibonacci hash
    {
        FibonacciHashN hasher(n);
        std::unordered_map<uint64_t, int> counts;
        for (int i = 0; i < num_tests; ++i) {
            std::string key = "test" + std::to_string(i);
            counts[hasher.hash(key)]++;
        }
        
        int collisions = 0;
        for (const auto& [h, count] : counts) {
            if (count > 1) collisions += count - 1;
        }
        
        std::log_info("Fibonacci hash (N={}): {} collisions", n, collisions);
    }
    
    std::log_info("\n=== Mathematical Insight ===");
    std::log_info("The golden ratio φ = (1 + √5) / 2 has special properties:");
    std::log_info("1. It's the 'most irrational' number - hardest to approximate with fractions");
    std::log_info("2. Powers of φ have maximum spacing when taken modulo 1");
    std::log_info("3. This translates to optimal distribution in hash functions");
    std::log_info("\nFor hash table of size N:");
    std::log_info("  Multiplier = nearest_prime(N / φ)");
    
    // Performance test for different bit sizes
    std::log_info("\n\n=== Performance Analysis for Different Bit Sizes ===");
    
    std::vector<std::pair<uint64_t, std::string>> perf_test_sizes = {
        {1009, "10-bit"},
        {65536, "16-bit"},
        {16777216, "24-bit"},
        {4294967291, "32-bit"},
        {1099511627776ULL, "40-bit"},
        {281474976710656ULL, "48-bit"}
    };
    
    const int perf_iterations = 1000000;
    std::string test_string = "benchmark_test_string_12345";
    
    for (const auto& [n, label] : perf_test_sizes) {
        if (n > 4294967295ULL && n != perf_test_sizes.back().first) {
            continue; // Skip very large sizes except the last one
        }
        
        GoldenHashN hasher(n);
        
        auto start = std::chrono::high_resolution_clock::now();
        uint64_t dummy = 0;
        
        for (int i = 0; i < perf_iterations; ++i) {
            dummy += hasher.hash(test_string + std::to_string(i));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        double ns_per_hash = static_cast<double>(duration.count()) / perf_iterations;
        double hashes_per_sec = 1e9 / ns_per_hash;
        
        std::log_info("{} hash (N={}): {:.2f} ns/hash, {:.2f} M hashes/sec", 
                      label, n, ns_per_hash, hashes_per_sec / 1e6);
        
        // Prevent optimization
        if (dummy == 0) std::log_info("");
    }
    
    std::log_info("\nConclusion: Hash performance is largely independent of table size!");
    std::log_info("The modulo operation at the end is the only size-dependent part.");
    
    return 0;
}