#pragma once

#include "map_shard.hpp"
#include <sqlite3.h>
#include <atomic>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstdio>

namespace goldenhash::tests {

/**
 * @brief SQLite shard for handling a range of hash values
 */
class SQLiteShard : public MapShard {
private:
    sqlite3* db;
    sqlite3_stmt* insert_stmt;
    sqlite3_stmt* select_stmt;
    sqlite3_stmt* update_stmt;
    uint64_t range_start;
    uint64_t range_end;
    uint64_t collision_count_{0};
    uint64_t max_count_{0};
    uint64_t unique_count_{0};
    std::atomic<bool> in_use{false};
public:

    SQLiteShard(std::string filename, int shard_id, uint64_t start, uint64_t end) 
        : range_start(start), range_end(end) {
        
        // Open SQLite database
        if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite shard: " + std::string(sqlite3_errmsg(db)));
        }
        
        // Create table if it doesn't exist
        const char* create_table = "CREATE TABLE IF NOT EXISTS hash_counts (hash INTEGER PRIMARY KEY, count INTEGER)";
        if (sqlite3_exec(db, create_table, nullptr, nullptr, nullptr) != SQLITE_OK) {
            sqlite3_close(db);
            throw std::runtime_error("Failed to create table in shard: " + std::string(sqlite3_errmsg(db)));
        }
        
        // Prepare statements
        sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO hash_counts (hash, count) VALUES (?, 1)", -1, &insert_stmt, nullptr);
        sqlite3_prepare_v2(db, "SELECT count FROM hash_counts WHERE hash = ?", -1, &select_stmt, nullptr);
        sqlite3_prepare_v2(db, "UPDATE hash_counts SET count = count + 1 WHERE hash = ?", -1, &update_stmt, nullptr);
        
        // SQLite special instructions for fast performance
        sqlite3_exec(db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    }
    
    ~SQLiteShard() {
        if (db) {
            // Get filename before closing
            const char* db_filename = sqlite3_db_filename(db, "main");
            std::string filename = db_filename ? db_filename : "";
            
            // Commit any pending transaction
            sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
            
            // Finalize statements
            sqlite3_finalize(insert_stmt);
            sqlite3_finalize(select_stmt);
            sqlite3_finalize(update_stmt);
            
            // Close database
            sqlite3_close(db);

            // Delete the database file if it was created (not in-memory)
            if (!filename.empty() && filename != ":memory:") {
                if (std::remove(filename.c_str()) != 0) {
                    std::cerr << "Failed to delete database file: " << filename << std::endl;
                }
            }
        }
    }

    // Non-copyable, non-movable
    SQLiteShard(const SQLiteShard&) = delete;
    SQLiteShard& operator=(const SQLiteShard&) = delete;
    SQLiteShard(SQLiteShard&&) = delete;
    SQLiteShard& operator=(SQLiteShard&&) = delete;
    
    bool process_hash(uint64_t hash) override {
        // Spinlock acquisition
        bool expected = false;
        while (!in_use.compare_exchange_weak(expected, true, 
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        // Check if hash exists
        sqlite3_bind_int64(select_stmt, 1, hash);
        int count = 0;
        if (sqlite3_step(select_stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(select_stmt, 0);
        }
        sqlite3_reset(select_stmt);
        if (count > 0) {
            // Update existing (collision!)
            collision_count_++;
            count++;  // Increment count
            sqlite3_bind_int64(update_stmt, 1, hash);
            sqlite3_step(update_stmt);
            sqlite3_reset(update_stmt);
            if (static_cast<uint64_t>(count) > max_count_) {
                max_count_ = count;
            }
        } else {
            // Insert new
            unique_count_++;
            sqlite3_bind_int64(insert_stmt, 1, hash);
            sqlite3_step(insert_stmt);
            sqlite3_reset(insert_stmt);
            if (1 > max_count_) {
                max_count_ = 1;
            }
        }
        // Commit periodically for better performance
        if (((collision_count_ + unique_count_) & 0x3FFF) == 0) {
            sqlite3_exec(db, "COMMIT; BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        }
        // Release spinlock
        in_use.store(false, std::memory_order_release);
        return count > 0;
    }
    
    void commit_batch() {
        bool expected = false;
        while (!in_use.compare_exchange_weak(expected, true)) {
            expected = false;
            std::this_thread::yield();
        }
        
        sqlite3_exec(db, "COMMIT; BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        
        in_use.store(false, std::memory_order_release);
    }

    uint64_t get_unique() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        uint64_t count = unique_count_;
        in_use.store(false, std::memory_order_release);
        return count;
    }

    uint64_t get_collisions() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true, 
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        uint64_t count = collision_count_;
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
        uint64_t count = max_count_;
        in_use.store(false, std::memory_order_release);
        return count;
    }
};

} // namespace goldenhash
