#pragma once

#include "test_data.hpp"
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
class SQLiteTestData : public TestData {
private:
    sqlite3* db;
    sqlite3_stmt* insert_stmt;
    sqlite3_stmt* select_stmt;
    std::atomic<bool> in_use{false};
public:
    SQLiteTestData(const std::string& filename) {
        // Open SQLite database
        if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open SQLite test data: " + std::string(sqlite3_errmsg(db)));
        }
        // Database special instructions for fast performance
        sqlite3_exec(db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        // Create table if it doesn't exist
        const char* create_table = "CREATE TABLE IF NOT EXISTS test_data (id INTEGER PRIMARY KEY, test TEXT)";
        if (sqlite3_exec(db, create_table, nullptr, nullptr, nullptr) != SQLITE_OK) {
            sqlite3_close(db);
            throw std::runtime_error("Failed to create table in SQLite test data: " + std::string(sqlite3_errmsg(db)));
        }
        // Commit the transaction to ensure the table is created
        sqlite3_exec(db, "COMMIT; BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        // Prepare insert statement
        sqlite3_prepare_v2(db, "INSERT INTO test_data (test) VALUES (?)", -1, &insert_stmt, nullptr);
        // Prepare select statement
        sqlite3_prepare_v2(db, "SELECT test FROM test_data WHERE id = ?", -1, &select_stmt, nullptr);
    }
    
    ~SQLiteTestData() {
        // Get filename before closing
        const char* db_filename = sqlite3_db_filename(db, "main");
        std::string filename = db_filename ? db_filename : "";
        
        sqlite3_finalize(insert_stmt);
        sqlite3_finalize(select_stmt);
        sqlite3_close(db);
        
        // Delete the database file if it was created (not in-memory)
        if (!filename.empty() && filename != ":memory:") {
            if (std::remove(filename.c_str()) != 0) {
                std::cerr << "Failed to delete database file: " << filename << std::endl;
            }
        }
    }
    
    void add_test(const std::string& test) override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        sqlite3_bind_text(insert_stmt, 1, test.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(insert_stmt) != SQLITE_DONE) {
            sqlite3_reset(insert_stmt);
            in_use.store(false, std::memory_order_release);
            throw std::runtime_error("Failed to insert test data: " + std::string(sqlite3_errmsg(db)));
        }
        sqlite3_reset(insert_stmt);
        in_use.store(false, std::memory_order_release);
    }
    
    void clean_up() override {
        // Clear the table
        sqlite3_exec(db, "DELETE FROM test_data", nullptr, nullptr, nullptr);
    }

    std::string get_test(size_t index) override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        
        sqlite3_bind_int(select_stmt, 1, static_cast<int>(index + 1)); // SQLite is 1-indexed
        if (sqlite3_step(select_stmt) == SQLITE_ROW) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt, 0));
            std::string result(text ? text : "");
            sqlite3_reset(select_stmt);
            in_use.store(false, std::memory_order_release);
            return result;
        } else {
            sqlite3_reset(select_stmt);
            in_use.store(false, std::memory_order_release);
            throw std::out_of_range("Index out of range");
        }
    }
    
    size_t size() override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        
        sqlite3_stmt* count_stmt;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM test_data", -1, &count_stmt, nullptr);
        size_t count = 0;
        if (sqlite3_step(count_stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(count_stmt, 0);
        }
        sqlite3_finalize(count_stmt);
        in_use.store(false, std::memory_order_release);
        return count;
    }
};

} // namespace goldenhash
