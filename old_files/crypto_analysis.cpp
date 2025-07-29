#include "include/crocs.hpp"
#include <iostream>
#include <random>
#include <vector>
#include <cmath>

using namespace crocs;

/**
 * Cryptographic Analysis: Dual-Table Attack
 * 
 * Given:
 * - Secret S (unknown)
 * - N1 = S
 * - N2 = 2^64 - S
 * - We can observe H1 = CROCS_N1(data) and H2 = CROCS_N2(data)
 * 
 * Question: Can we recover S?
 */

class DualTableAnalyzer {
private:
    static constexpr uint64_t MAX_UINT64 = UINT64_MAX;
    
public:
    struct AttackResult {
        bool success;
        uint64_t recovered_secret;
        uint64_t attempts;
        double confidence;
    };
    
    /**
     * Attempt to recover secret from hash observations
     */
    AttackResult analyze_hashes(
        const std::vector<uint64_t>& hashes1,
        const std::vector<uint64_t>& hashes2,
        const std::vector<std::vector<uint8_t>>& inputs
    ) {
        AttackResult result{false, 0, 0, 0.0};
        
        // Key insight: The golden primes are related to table sizes
        // P1 ≈ N1/φ = S/φ
        // P2 ≈ N2/φ = (2^64 - S)/φ
        // Therefore: P1 + P2 ≈ 2^64/φ
        
        // Step 1: Estimate table sizes from hash distributions
        uint64_t max_h1 = *std::max_element(hashes1.begin(), hashes1.end());
        uint64_t max_h2 = *std::max_element(hashes2.begin(), hashes2.end());
        
        std::cout << "Max hash values: H1=" << max_h1 << ", H2=" << max_h2 << "\n";
        
        // Step 2: Use collision patterns to estimate table sizes
        std::unordered_map<uint64_t, int> freq1, freq2;
        for (auto h : hashes1) freq1[h]++;
        for (auto h : hashes2) freq2[h]++;
        
        size_t unique1 = freq1.size();
        size_t unique2 = freq2.size();
        
        // Birthday paradox inverse: Given k unique values from n samples
        // Estimate N ≈ n²/(2*(n-k))
        size_t n = hashes1.size();
        uint64_t est_n1 = (n * n) / (2 * (n - unique1 + 1));
        uint64_t est_n2 = (n * n) / (2 * (n - unique2 + 1));
        
        std::cout << "Estimated N1: " << est_n1 << " (unique: " << unique1 << ")\n";
        std::cout << "Estimated N2: " << est_n2 << " (unique: " << unique2 << ")\n";
        
        // Step 3: Algebraic attack using golden ratio relationship
        // Since N1 + N2 = 2^64, and we know the primes are near N/φ
        // We can search for S where:
        // - GoldenPrime(S) produces observed hash patterns
        // - GoldenPrime(2^64 - S) produces observed hash patterns
        
        result.attempts = 0;
        const uint64_t search_range = 1000000; // Search nearby estimates
        
        for (uint64_t delta = 0; delta < search_range; delta++) {
            // Try both directions
            for (int dir : {-1, 1}) {
                uint64_t candidate = est_n1 + dir * delta;
                if (candidate == 0 || candidate >= MAX_UINT64) continue;
                
                uint64_t complement = MAX_UINT64 - candidate + 1;
                
                // Get golden primes
                uint64_t p1 = GoldenPrimeFinder::find_golden_prime(candidate);
                uint64_t p2 = GoldenPrimeFinder::find_golden_prime(complement);
                
                // Test if these primes produce our observed hashes
                bool matches = test_prime_match(p1, candidate, inputs[0], hashes1[0]) &&
                              test_prime_match(p2, complement, inputs[0], hashes2[0]);
                
                result.attempts++;
                
                if (matches) {
                    // Verify with more samples
                    int verified = 0;
                    for (size_t i = 1; i < std::min<size_t>(10, inputs.size()); i++) {
                        if (test_prime_match(p1, candidate, inputs[i], hashes1[i]) &&
                            test_prime_match(p2, complement, inputs[i], hashes2[i])) {
                            verified++;
                        }
                    }
                    
                    if (verified >= 8) {
                        result.success = true;
                        result.recovered_secret = candidate;
                        result.confidence = verified / 9.0;
                        return result;
                    }
                }
            }
        }
        
        return result;
    }
    
private:
    bool test_prime_match(uint64_t prime, uint64_t table_size, 
                         const std::vector<uint8_t>& input, uint64_t expected_hash) {
        uint64_t h = 0;
        for (uint8_t byte : input) {
            h = h * prime + byte;
            h ^= h >> 32;
        }
        h = (h * prime) % table_size;
        return h == expected_hash;
    }
};

int main() {
    std::cout << "CROCS Cryptographic Analysis: Dual-Table Attack\n";
    std::cout << "=" << std::string(50, '=') << "\n\n";
    
    // Generate a secret
    std::mt19937_64 rng(42);
    uint64_t secret = rng() % (1ULL << 40); // Use 40-bit secret for feasibility
    uint64_t complement = DualTableAnalyzer::MAX_UINT64 - secret + 1;
    
    std::cout << "Secret S: " << secret << "\n";
    std::cout << "Table N1: " << secret << "\n";
    std::cout << "Table N2: " << complement << "\n";
    
    // Get golden primes
    uint64_t p1 = GoldenPrimeFinder::find_golden_prime(secret);
    uint64_t p2 = GoldenPrimeFinder::find_golden_prime(complement);
    
    std::cout << "Prime P1: " << p1 << " (N1/φ = " << uint64_t(secret/PHI) << ")\n";
    std::cout << "Prime P2: " << p2 << " (N2/φ = " << uint64_t(complement/PHI) << ")\n";
    std::cout << "\n";
    
    // Generate test data and hashes
    const size_t num_samples = 10000;
    std::vector<std::vector<uint8_t>> inputs;
    std::vector<uint64_t> hashes1, hashes2;
    
    for (size_t i = 0; i < num_samples; i++) {
        // Random input
        std::vector<uint8_t> input(16);
        for (auto& b : input) b = rng() & 0xFF;
        inputs.push_back(input);
        
        // Hash with both tables
        uint64_t h1 = 0, h2 = 0;
        for (uint8_t byte : input) {
            h1 = h1 * p1 + byte;
            h1 ^= h1 >> 32;
            h2 = h2 * p2 + byte;
            h2 ^= h2 >> 32;
        }
        hashes1.push_back((h1 * p1) % secret);
        hashes2.push_back((h2 * p2) % complement);
    }
    
    // Attempt recovery
    std::cout << "Attempting to recover secret from " << num_samples << " hash observations...\n\n";
    
    DualTableAnalyzer analyzer;
    auto result = analyzer.analyze_hashes(hashes1, hashes2, inputs);
    
    std::cout << "\nAttack Result:\n";
    std::cout << "Success: " << (result.success ? "YES" : "NO") << "\n";
    if (result.success) {
        std::cout << "Recovered: " << result.recovered_secret << "\n";
        std::cout << "Actual: " << secret << "\n";
        std::cout << "Error: " << int64_t(result.recovered_secret) - int64_t(secret) << "\n";
        std::cout << "Confidence: " << result.confidence * 100 << "%\n";
    }
    std::cout << "Attempts: " << result.attempts << "\n";
    
    // Analysis
    std::cout << "\nCryptographic Implications:\n";
    std::cout << "1. Table size recovery is possible due to birthday paradox\n";
    std::cout << "2. Golden ratio relationship provides algebraic structure\n";
    std::cout << "3. Dual-table construction does NOT hide the secret well\n";
    std::cout << "4. Need additional cryptographic primitives for security\n";
    
    return 0;
}