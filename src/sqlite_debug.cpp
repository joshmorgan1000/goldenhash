#include <iostream>
#include <goldenhash/tests/sqlite_shard.hpp>

int main() {
    goldenhash::tests::SQLiteShard shard("debug_shard.db", 0, 0, UINT64_MAX);
    
    // Test same hash multiple times
    uint64_t test_hash = 12345;
    
    std::cout << "Initial state:\n";
    std::cout << "Unique: " << shard.get_unique() << ", Collisions: " << shard.get_collisions() << ", Max: " << shard.get_max_count() << "\n\n";
    
    // First occurrence
    bool collision1 = shard.process_hash(test_hash);
    std::cout << "After first hash (12345):\n";
    std::cout << "Collision: " << collision1 << ", Unique: " << shard.get_unique() << ", Collisions: " << shard.get_collisions() << ", Max: " << shard.get_max_count() << "\n\n";
    
    // Second occurrence (should be a collision)
    bool collision2 = shard.process_hash(test_hash);
    std::cout << "After second hash (12345):\n";
    std::cout << "Collision: " << collision2 << ", Unique: " << shard.get_unique() << ", Collisions: " << shard.get_collisions() << ", Max: " << shard.get_max_count() << "\n\n";
    
    // Different hash
    bool collision3 = shard.process_hash(67890);
    std::cout << "After third hash (67890):\n";
    std::cout << "Collision: " << collision3 << ", Unique: " << shard.get_unique() << ", Collisions: " << shard.get_collisions() << ", Max: " << shard.get_max_count() << "\n\n";
    
    // Third occurrence of first hash
    bool collision4 = shard.process_hash(test_hash);
    std::cout << "After fourth hash (12345):\n";
    std::cout << "Collision: " << collision4 << ", Unique: " << shard.get_unique() << ", Collisions: " << shard.get_collisions() << ", Max: " << shard.get_max_count() << "\n\n";
    
    return 0;
}