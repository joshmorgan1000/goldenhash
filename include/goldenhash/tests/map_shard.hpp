#pragma once

#include <unordered_map>
#include <atomic>
#include <thread>

namespace goldenhash::tests {

class MapShard {
public:
    virtual ~MapShard() = default;
    /**
     * @brief Process a hash value
     * @param hash Hash value to process
     * @return true if there was a collision, false otherwise
     */
    virtual bool process_hash(uint64_t hash) = 0;
    virtual uint64_t get_unique() = 0;
    virtual uint64_t get_collisions() = 0;
    virtual uint64_t get_max_count() = 0;
};

/**
 * @brief Hash map shard for multi-threaded in-memory processing
 */
class HashMapShard : public MapShard {
private:
    std::unordered_map<uint64_t, uint32_t> map;
    std::atomic<bool> in_use{false};
    uint64_t unique_{0};
    uint64_t collisions_{0};
    uint64_t max_count{0};
public:
    HashMapShard() = default;
    HashMapShard(const HashMapShard&) = delete;
    HashMapShard& operator=(const HashMapShard&) = delete;
    HashMapShard(HashMapShard&&) = delete;
    HashMapShard& operator=(HashMapShard&&) = delete;
    ~HashMapShard() {
        // Destructor can be empty as we don't own the map memory
    }
    
    bool process_hash(uint64_t hash) override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        uint32_t& count = map[hash];
        count++;
        bool collision = false;
        if (count > 1) {
            collisions_++;
            collision = true;
        } else {
            unique_++;
        }
        if (count > max_count) {
            max_count = count;
        }
        in_use.store(false, std::memory_order_release);
        return collision;
    }

    uint64_t get_unique() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        // Return the actual number of unique hash values (size of the map)
        uint64_t unique = map.size();
        in_use.store(false, std::memory_order_release);
        return unique;
    }

    uint64_t get_collisions() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        uint64_t count = collisions_;
        in_use.store(false, std::memory_order_release);
        return count;
    }
    
    uint64_t get_max_count() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        uint64_t count = max_count;
        in_use.store(false, std::memory_order_release);
        return count;
    }
};

} // namespace goldenhash
