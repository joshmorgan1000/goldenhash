/**
 * @file float_hash.hpp
 * @brief Floating-point based hash function using golden ratio
 * @author Josh Morgan
 * @date 2025
 * 
 * This file implements a deterministic hash function that uses floating-point
 * arithmetic with the golden ratio for entropy-rich hash generation.
 */

#ifndef FLOAT_HASH_HPP
#define FLOAT_HASH_HPP

#include "inmemory_hash.hpp"
#include <cfenv>
#include <cmath>

namespace goldenhash {

/**
 * @class FloatHash
 * @brief Hash function implementation using floating-point arithmetic and golden ratio
 * 
 * This hash function leverages the golden ratio's mathematical properties
 * combined with floating-point operations to create a fast, deterministic,
 * and entropy-rich hash function that is sensitive to table size.
 */
class FloatHash : public InMemoryHash {
private:
    static constexpr double GOLDEN_RATIO = 1.6180339887498948482;
    static constexpr uint64_t PRIME_MULTIPLIER = 0x9e3779b97f4a7c15ULL; // Based on golden ratio
    
public:
    /**
     * @brief Constructor
     */
    FloatHash() = default;
    
    /**
     * @brief Hash a single key
     * @param key The key to hash
     * @param table_size The size of the hash table
     * @return The hash value
     */
    uint64_t hash(uint64_t key, uint64_t table_size) const override {
        return hash_with_seed(key, 0, table_size);
    }
    
    /**
     * @brief Hash a single key with a seed
     * @param key The key to hash
     * @param seed The seed value for the hash function
     * @param table_size The size of the hash table
     * @return The hash value
     * 
     * This implementation ensures deterministic rounding behavior and uses
     * the golden ratio to distribute hash values uniformly across the table.
     */
    uint64_t hash_with_seed(uint64_t key, uint64_t seed, uint64_t table_size) const override {
        // Ensure deterministic rounding
        #ifdef FE_TONEAREST
        std::fesetround(FE_TONEAREST);
        #endif
        
        // Mix key with seed using XOR and prime multiplication
        uint64_t mixed_key = (key ^ seed) * PRIME_MULTIPLIER;
        
        // Convert to double and multiply by golden ratio and table size
        double x = static_cast<double>(mixed_key);
        double mix = x * GOLDEN_RATIO * static_cast<double>(table_size);
        
        // Extract fractional part for uniform distribution in [0, 1)
        double frac = mix - std::floor(mix);
        
        // Scale to full 64-bit range
        uint64_t full_hash = static_cast<uint64_t>(frac * static_cast<double>(UINT64_MAX));
        
        // Final modulo to ensure result is within table bounds
        return full_hash % table_size;
    }
    
    /**
     * @brief Get the name of the hash function
     * @return String containing the hash function name
     */
    std::string get_name() const override {
        return "FloatHash";
    }
    
    /**
     * @brief Get a description of the hash function
     * @return String containing the hash function description
     */
    std::string get_description() const override {
        return "Golden ratio-based floating-point hash function with deterministic rounding";
    }
    
    /**
     * @brief Check if the hash function supports seeding
     * @return true
     */
    bool supports_seed() const override {
        return true;
    }
    
    /**
     * @brief Check if the hash function uses floating-point arithmetic
     * @return true
     */
    bool uses_floating_point() const override {
        return true;
    }
    
    /**
     * @brief Get the recommended minimum table size
     * @return Minimum recommended table size (2)
     */
    uint64_t get_min_table_size() const override {
        return 2;
    }
    
    /**
     * @brief Get the recommended maximum table size
     * @return Maximum recommended table size
     */
    uint64_t get_max_table_size() const override {
        // Limit to ensure floating-point precision is maintained
        return 1ULL << 53; // 2^53 - largest integer exactly representable in double
    }
};

} // namespace goldenhash

#endif // FLOAT_HASH_HPP