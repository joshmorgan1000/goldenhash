#pragma once
/**
 * @file goldenhash.hpp
 * @brief Header file for the CROCS golden ratio hash function
 * @details This file defines the GoldenHash class which implements a hash function
 * based on primes and the golden ratio. It aims to efficiently generate a high
 * performance hash function for any N size hash table. We expect it to hit all the
 * metrics - Chi-square, collision rate, avalanche effect, and the birthday paradox.
 */
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <span>
#include <bit>
#include <concepts>
#include <type_traits>
#include <functional>
#include <cstdlib>

namespace goldenhash {

// Golden ratio constant
inline constexpr double PHI = 1.6180339887498948482;

// Concept for hashable types
template<typename T>
concept Hashable = requires(T t) {
    { std::data(t) } -> std::convertible_to<const void*>;
    { std::size(t) } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Optimized primality test for large numbers
 */
class PrimalityTester {
public:
    static bool is_prime(uint64_t n) {
        if (n < 2) return false;
        if (n == 2 || n == 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        
        // Check up to sqrt(n) with 6k±1 optimization
        uint64_t sqrt_n = static_cast<uint64_t>(std::sqrt(n));
        for (uint64_t i = 5; i <= sqrt_n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    }
    
    // Miller-Rabin test for very large numbers
    static bool is_prime_miller_rabin(uint64_t n, int k = 5) {
        if (n < 2) return false;
        if (n == 2 || n == 3) return true;
        if (n % 2 == 0) return false;
        
        // Write n-1 as 2^r * d
        uint64_t r = 0, d = n - 1;
        while (d % 2 == 0) {
            r++;
            d /= 2;
        }
        
        // Witness loop
        for (int i = 0; i < k; i++) {
            uint64_t a = 2 + (rand() % (n - 3));
            uint64_t x = mod_pow(a, d, n);
            
            if (x == 1 || x == n - 1) continue;
            
            bool composite = true;
            for (uint64_t j = 0; j < r - 1; j++) {
                x = (x * x) % n;
                if (x == n - 1) {
                    composite = false;
                    break;
                }
            }
            
            if (composite) return false;
        }
        return true;
    }
    
private:
    static uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
        uint64_t result = 1;
        base %= mod;
        while (exp > 0) {
            if (exp & 1) result = (__uint128_t)result * base % mod;
            base = (__uint128_t)base * base % mod;
            exp >>= 1;
        }
        return result;
    }
};

/**
 * @brief Golden ratio prime finder
 */
class GoldenPrimeFinder {
public:
    /**
     * @brief Find the optimal prime for a given hash table size
     * @param n The size of the hash table
     * @return The nearest prime to n/φ
     */
    static uint64_t find_golden_prime(uint64_t n) {
        uint64_t golden_value = static_cast<uint64_t>(n / PHI);
        
        // For very large n, use more efficient search
        if (n > 1ULL << 32) {
            return find_large_prime_near(golden_value);
        }
        
        // Standard search for smaller values
        return find_nearest_prime(golden_value, n);
    }
    
private:
    static uint64_t find_nearest_prime(uint64_t target, uint64_t max_value) {
        if (target > max_value) target = max_value;
        
        if (PrimalityTester::is_prime(target)) return target;
        
        // Search outward from target
        for (uint64_t delta = 1; delta < target && delta < 10000; ++delta) {
            if (target > delta && PrimalityTester::is_prime(target - delta)) {
                return target - delta;
            }
            if (target + delta <= max_value && PrimalityTester::is_prime(target + delta)) {
                return target + delta;
            }
        }
        
        // Fallback for very small n
        return 2;
    }
    
    static uint64_t find_large_prime_near(uint64_t target) {
        // For very large numbers, check fewer candidates
        if (PrimalityTester::is_prime_miller_rabin(target)) return target;
        
        // Only check odd numbers
        if (target % 2 == 0) target--;
        
        for (uint64_t candidate = target; candidate > target - 1000; candidate -= 2) {
            if (PrimalityTester::is_prime_miller_rabin(candidate)) {
                return candidate;
            }
        }
        
        for (uint64_t candidate = target + 2; candidate < target + 1000; candidate += 2) {
            if (PrimalityTester::is_prime_miller_rabin(candidate)) {
                return candidate;
            }
        }
        
        return target; // Give up and use non-prime
    }
};

/**
 * @brief CROCS hash function for arbitrary bit sizes
 * @tparam OutputBits Number of output bits (8, 16, 24, 32, 64, etc.)
 */
template<size_t OutputBits>
class CrocsHash {
    static_assert(OutputBits >= 8 && OutputBits <= 64, 
                  "Output bits must be between 8 and 64");
    
private:
    using OutputType = std::conditional_t<
        OutputBits <= 8,  uint8_t,
        std::conditional_t<
            OutputBits <= 16, uint16_t,
            std::conditional_t<
                OutputBits <= 32, uint32_t,
                uint64_t
            >
        >
    >;
    
    static constexpr uint64_t OUTPUT_MASK = (OutputBits == 64) ? 
        ~0ULL : ((1ULL << OutputBits) - 1);
    
    uint64_t prime_;
    uint64_t n_;
    
public:
    explicit CrocsHash(uint64_t table_size) : n_(table_size) {
        prime_ = GoldenPrimeFinder::find_golden_prime(n_);
    }
    
    /**
     * @brief Hash arbitrary data
     */
    template<Hashable T>
    OutputType hash(const T& data) const {
        return hash_bytes(std::data(data), std::size(data));
    }
    
    /**
     * @brief Hash raw bytes
     */
    OutputType hash_bytes(const void* data, size_t len) const {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        uint64_t h = 0;
        
        // Main hash loop with mixing
        constexpr int shift_amount = OutputBits / 2;
        
        for (size_t i = 0; i < len; ++i) {
            h = h * prime_ + bytes[i];
            h ^= h >> shift_amount;  // Mix bits
        }
        
        // Final mixing
        h *= prime_;
        h ^= h >> (OutputBits - OutputBits / 3);
        
        // Reduce to output size
        return static_cast<OutputType>((h & OUTPUT_MASK) % n_);
    }
    
    uint64_t get_prime() const { return prime_; }
    uint64_t get_table_size() const { return n_; }
};

/**
 * @brief Convenience aliases for common bit sizes
 */
using CrocsHash8  = CrocsHash<8>;
using CrocsHash16 = CrocsHash<16>;
using CrocsHash24 = CrocsHash<24>;
using CrocsHash32 = CrocsHash<32>;
using CrocsHash48 = CrocsHash<48>;
using CrocsHash64 = CrocsHash<64>;

/**
 * @brief Advanced CROCS hash with customizable mixing functions
 */
template<size_t OutputBits>
class CrocsHashAdvanced : public CrocsHash<OutputBits> {
public:
    using MixFunction = std::function<uint64_t(uint64_t, int)>;
    using FinalizeFunction = std::function<uint64_t(uint64_t, uint64_t)>;
    
private:
    MixFunction mixer_;
    FinalizeFunction finalizer_;
    
public:
    CrocsHashAdvanced(uint64_t table_size,
                      MixFunction mixer = nullptr,
                      FinalizeFunction finalizer = nullptr) 
        : CrocsHash<OutputBits>(table_size) {
        
        if (!mixer) {
            mixer_ = [](uint64_t h, int bits) {
                return h ^ (h >> (bits / 2));
            };
        } else {
            mixer_ = mixer;
        }
        
        if (!finalizer) {
            finalizer_ = [](uint64_t h, uint64_t prime) {
                h *= prime;
                h ^= h >> 27;
                return h;
            };
        } else {
            finalizer_ = finalizer;
        }
    }
    
    // Additional methods for experimentation...
};

} // namespace goldenhash
