/**
 * @file inmemory_hash.hpp
 * @brief Base class for in-memory hash functions
 * @author Josh Morgan
 * @date 2025
 * 
 * This file defines the base interface for all in-memory hash functions.
 * Derived classes should implement the pure virtual methods to provide
 * specific hashing algorithms.
 */

#ifndef INMEMORY_HASH_HPP
#define INMEMORY_HASH_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace goldenhash {

/**
 * @class InMemoryHash
 * @brief Abstract base class for in-memory hash functions
 * 
 * This class defines the interface that all hash function implementations
 * must follow. It provides both single-key and batch hashing capabilities,
 * as well as metadata about the hash function.
 */
class InMemoryHash {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~InMemoryHash() = default;
    
    /**
     * @brief Hash a single key
     * @param key The key to hash
     * @param table_size The size of the hash table
     * @return The hash value
     */
    virtual uint64_t hash(uint64_t key, uint64_t table_size) const = 0;
    
    /**
     * @brief Hash a single key with a seed
     * @param key The key to hash
     * @param seed The seed value for the hash function
     * @param table_size The size of the hash table
     * @return The hash value
     */
    virtual uint64_t hash_with_seed(uint64_t key, uint64_t seed, uint64_t table_size) const = 0;
    
    /**
     * @brief Batch hash multiple keys
     * @param keys Vector of keys to hash
     * @param table_size The size of the hash table
     * @return Vector of hash values
     */
    virtual std::vector<uint64_t> hash_batch(const std::vector<uint64_t>& keys, uint64_t table_size) const {
        std::vector<uint64_t> results;
        results.reserve(keys.size());
        for (const auto& key : keys) {
            results.push_back(hash(key, table_size));
        }
        return results;
    }
    
    /**
     * @brief Batch hash multiple keys with a seed
     * @param keys Vector of keys to hash
     * @param seed The seed value for the hash function
     * @param table_size The size of the hash table
     * @return Vector of hash values
     */
    virtual std::vector<uint64_t> hash_batch_with_seed(const std::vector<uint64_t>& keys, 
                                                       uint64_t seed, 
                                                       uint64_t table_size) const {
        std::vector<uint64_t> results;
        results.reserve(keys.size());
        for (const auto& key : keys) {
            results.push_back(hash_with_seed(key, seed, table_size));
        }
        return results;
    }
    
    /**
     * @brief Get the name of the hash function
     * @return String containing the hash function name
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Get a description of the hash function
     * @return String containing the hash function description
     */
    virtual std::string get_description() const = 0;
    
    /**
     * @brief Check if the hash function supports seeding
     * @return true if seeding is supported, false otherwise
     */
    virtual bool supports_seed() const = 0;
    
    /**
     * @brief Check if the hash function uses floating-point arithmetic
     * @return true if floating-point is used, false otherwise
     */
    virtual bool uses_floating_point() const = 0;
    
    /**
     * @brief Get the recommended minimum table size
     * @return Minimum recommended table size
     */
    virtual uint64_t get_min_table_size() const {
        return 1;
    }
    
    /**
     * @brief Get the recommended maximum table size
     * @return Maximum recommended table size
     */
    virtual uint64_t get_max_table_size() const {
        return UINT64_MAX;
    }
};

} // namespace goldenhash

#endif // INMEMORY_HASH_HPP