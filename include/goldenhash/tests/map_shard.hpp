#pragma once

#include <unordered_map>
#include <map>
#include <atomic>
#include <thread>

namespace goldenhash::tests {

class MapShard {
public:
    virtual ~MapShard() = default;
    /**
     * @brief Process a hash value (for performance testing only)
     * @param hash Hash value to process
     */
    virtual void process_hash(uint64_t hash) = 0;
};

/**
 * @brief Hash map shard for multi-threaded in-memory processing
 */
class HashMapShard : public MapShard {
private:
    std::unordered_map<uint64_t, uint32_t> map;
    std::atomic<bool> in_use{false};
public:
    HashMapShard() = default;
    HashMapShard(const HashMapShard&) = delete;
    HashMapShard& operator=(const HashMapShard&) = delete;
    HashMapShard(HashMapShard&&) = delete;
    HashMapShard& operator=(HashMapShard&&) = delete;
    ~HashMapShard() = default;
    
    void process_hash(uint64_t hash) override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        // Simply store the hash for memory pressure simulation
        map[hash]++;
        in_use.store(false, std::memory_order_release);
    }
};

} // namespace goldenhash
