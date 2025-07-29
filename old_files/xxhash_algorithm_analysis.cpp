#include <iostream>
#include <cstdint>
#include <cstring>
#include <bit>

// xxHash64 algorithm analysis
class XXHash64Analysis {
    static constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    static constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    static constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    static constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    static constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;
    
public:
    static void show_algorithm() {
        std::cout << "xxHash64 Algorithm Structure:\n";
        std::cout << "=============================\n\n";
        
        // Step 1: Main loop (processes 32 bytes at a time)
        std::cout << "1. MAIN LOOP (32 bytes at a time):\n";
        std::cout << "   v1 = seed + PRIME64_1 + PRIME64_2\n";
        std::cout << "   v2 = seed + PRIME64_2\n";
        std::cout << "   v3 = seed + 0\n";
        std::cout << "   v4 = seed - PRIME64_1\n\n";
        
        std::cout << "   For each 8-byte chunk:\n";
        std::cout << "   v[i] = round(v[i], chunk)\n";
        std::cout << "   where round(acc, input) = {\n";
        std::cout << "       acc = acc + input * PRIME64_2\n";
        std::cout << "       acc = rotl(acc, 31)\n";
        std::cout << "       acc = acc * PRIME64_1\n";
        std::cout << "   }\n\n";
        
        // Step 2: Convergence
        std::cout << "2. CONVERGENCE:\n";
        std::cout << "   h64 = rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18)\n";
        std::cout << "   h64 = mergeRound(h64, v1)\n";
        std::cout << "   h64 = mergeRound(h64, v2)\n";
        std::cout << "   h64 = mergeRound(h64, v3)\n";
        std::cout << "   h64 = mergeRound(h64, v4)\n\n";
        
        std::cout << "   where mergeRound(acc, val) = {\n";
        std::cout << "       acc ^= round(0, val)\n";
        std::cout << "       acc = acc * PRIME64_1 + PRIME64_4\n";
        std::cout << "   }\n\n";
        
        // Step 3: Remaining bytes
        std::cout << "3. REMAINING BYTES:\n";
        std::cout << "   8 bytes: h64 ^= round(0, read64)\n";
        std::cout << "   4 bytes: h64 ^= read32 * PRIME64_1; rotl(h64, 23) * PRIME64_2 + PRIME64_3\n";
        std::cout << "   1 byte:  h64 ^= read8 * PRIME64_5; rotl(h64, 11) * PRIME64_1\n\n";
        
        // Step 4: Final avalanche
        std::cout << "4. FINAL AVALANCHE:\n";
        std::cout << "   h64 ^= h64 >> 33\n";
        std::cout << "   h64 *= PRIME64_2\n";
        std::cout << "   h64 ^= h64 >> 29\n";
        std::cout << "   h64 *= PRIME64_3\n";
        std::cout << "   h64 ^= h64 >> 32\n";
    }
    
    static void show_scaling_pattern() {
        std::cout << "\n\nSCALING TO ARBITRARY OUTPUT SIZE:\n";
        std::cout << "==================================\n\n";
        
        std::cout << "Pattern discovered:\n";
        std::cout << "1. Each prime serves a specific PURPOSE:\n";
        std::cout << "   PRIME1: Main accumulator multiplier (golden ratio)\n";
        std::cout << "   PRIME2: Input mixer\n";
        std::cout << "   PRIME3: Offset adder\n";
        std::cout << "   PRIME4: Convergence offset\n";
        std::cout << "   PRIME5: Single byte multiplier\n\n";
        
        std::cout << "2. To scale to N bits:\n";
        std::cout << "   PRIME_N_1 = nearest_prime(2^N / φ)\n";
        std::cout << "   PRIME_N_2 = nearest_prime(2^N / φ²)\n";
        std::cout << "   PRIME_N_3 = smaller related prime\n";
        std::cout << "   PRIME_N_4 = derived from PRIME_N_1\n";
        std::cout << "   PRIME_N_5 = small multiplier\n\n";
        
        std::cout << "3. Key operations scale:\n";
        std::cout << "   - Multiply by golden ratio prime\n";
        std::cout << "   - Rotate by (N/2 - 1) or similar\n";
        std::cout << "   - XOR-shift by N/3, N/2, etc.\n";
    }
    
    static void demo_scaled_hash(int bits) {
        std::cout << "\n\nExample: " << bits << "-bit hash\n";
        std::cout << "================\n";
        
        uint64_t max_val = (bits == 64) ? UINT64_MAX : ((1ULL << bits) - 1);
        double phi = 1.6180339887498948482;
        
        uint64_t prime1 = max_val / phi;
        uint64_t prime2 = max_val / (phi * phi);
        
        // Make them odd
        prime1 |= 1;
        prime2 |= 1;
        
        std::cout << "Max value: " << max_val << " (2^" << bits << " - 1)\n";
        std::cout << "PRIME1: " << prime1 << " ≈ 2^" << bits << "/φ\n";
        std::cout << "PRIME2: " << prime2 << " ≈ 2^" << bits << "/φ²\n";
        std::cout << "Rotation: " << (bits/2 - 1) << " bits\n";
        std::cout << "XOR shifts: " << bits/3 << ", " << bits/2 << " bits\n";
    }
};

int main() {
    XXHash64Analysis::show_algorithm();
    XXHash64Analysis::show_scaling_pattern();
    
    // Show scaling examples
    XXHash64Analysis::demo_scaled_hash(32);
    XXHash64Analysis::demo_scaled_hash(48);
    XXHash64Analysis::demo_scaled_hash(64);
    
    std::cout << "\n\nCONCLUSION:\n";
    std::cout << "===========\n";
    std::cout << "xxHash uses MULTIPLE operations with golden ratio primes:\n";
    std::cout << "1. Multiply input by PRIME2 (spreads bits)\n";
    std::cout << "2. Rotate (mixes high/low bits)\n";
    std::cout << "3. Multiply by PRIME1 (golden ratio mixing)\n";
    std::cout << "4. Multiple rounds ensure thorough mixing\n";
    std::cout << "5. Different primes for different stages\n\n";
    
    std::cout << "This explains the good avalanche effect - it's not just\n";
    std::cout << "one multiply, but a carefully designed sequence!\n";
    
    return 0;
}