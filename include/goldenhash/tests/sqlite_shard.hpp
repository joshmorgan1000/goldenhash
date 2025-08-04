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
    std::atomic<bool> in_use{false};
public:

    SQLiteShard(std::string filename) {
        
        // Open SQLite database
        if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite shard: " + std::string(sqlite3_errmsg(db)));
        }
        
        // Create table if it doesn't exist
        // Use BLOB for hash to handle full 64-bit unsigned range
        const char* create_table = "CREATE TABLE IF NOT EXISTS hash_counts (hash BLOB PRIMARY KEY, count INTEGER)";
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
    
    void process_hash(uint64_t hash) override {
        // Spinlock acquisition
        bool expected = false;
        while (!in_use.compare_exchange_weak(expected, true, 
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        // Try to insert or update
        sqlite3_bind_blob(insert_stmt, 1, &hash, sizeof(hash), SQLITE_STATIC);
        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            // Already exists, update count
            sqlite3_reset(insert_stmt);
            sqlite3_bind_blob(update_stmt, 1, &hash, sizeof(hash), SQLITE_STATIC);
            sqlite3_step(update_stmt);
            sqlite3_reset(update_stmt);
        } else {
            sqlite3_reset(insert_stmt);
        }
        // Release spinlock
        in_use.store(false, std::memory_order_release);
    }
};

} // namespace goldenhash
