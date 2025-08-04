/**
 * @file collision_store.hpp
 * @brief Storage system for hash collision data and metadata
 * @author Josh Morgan
 * @date 2025
 * 
 * Provides interfaces and implementations for storing collision data
 * to disk for large-scale hash function analysis.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <sstream>
#include <iomanip>

namespace goldenhash::tests {

/**
 * @brief Represents a single hash collision
 */
struct CollisionRecord {
    uint64_t hash_value;
    std::vector<uint8_t> input1;
    std::vector<uint8_t> input2;
    size_t input1_index;
    size_t input2_index;
    uint64_t timestamp;
    std::string algorithm;
    uint64_t table_size;
};

/**
 * @brief Represents a test run with metrics
 */
struct TestRunRecord {
    std::string run_id;
    std::string algorithm;
    uint64_t table_size;
    uint64_t num_hashes;
    uint64_t timestamp;
    
    // Metrics
    double avalanche_score;
    double chi_squared;
    double collision_ratio;
    size_t actual_collisions;
    double expected_collisions;
    double throughput_mbs;
    double ns_per_hash;
    
    // Additional metadata
    std::string metadata_json;
};

/**
 * @brief Interface for collision storage
 */
class CollisionStore {
public:
    virtual ~CollisionStore() = default;
    
    /**
     * @brief Initialize the storage (create tables, etc.)
     * @return True on success
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Store a collision record
     * @param record Collision to store
     * @return True on success
     */
    virtual bool store_collision(const CollisionRecord& record) = 0;
    
    /**
     * @brief Store multiple collisions in a batch
     * @param records Collisions to store
     * @return Number of records successfully stored
     */
    virtual size_t store_collisions_batch(const std::vector<CollisionRecord>& records) = 0;
    
    /**
     * @brief Store a test run record with metrics
     * @param record Test run to store
     * @return True on success
     */
    virtual bool store_test_run(const TestRunRecord& record) = 0;
    
    /**
     * @brief Query collisions for a specific hash value
     * @param hash_value Hash to search for
     * @param algorithm Optional algorithm filter
     * @return Vector of collision records
     */
    virtual std::vector<CollisionRecord> query_collisions(uint64_t hash_value, 
                                                          const std::string& algorithm = "") = 0;
    
    /**
     * @brief Get collision statistics for a test run
     * @param run_id Test run identifier
     * @return Test run record if found
     */
    virtual std::optional<TestRunRecord> get_test_run(const std::string& run_id) = 0;
    
    /**
     * @brief Get all test runs for an algorithm
     * @param algorithm Algorithm name
     * @param limit Maximum number of results
     * @return Vector of test run records
     */
    virtual std::vector<TestRunRecord> get_test_runs(const std::string& algorithm, 
                                                     size_t limit = 100) = 0;
};

/**
 * @brief SQLite implementation of collision storage
 */
class SQLiteCollisionStore : public CollisionStore {
private:
    std::string db_path_;
    sqlite3* db_ = nullptr;
    
    // Prepared statements for performance
    sqlite3_stmt* insert_collision_stmt_ = nullptr;
    sqlite3_stmt* insert_test_run_stmt_ = nullptr;
    
public:
    /**
     * @brief Constructor
     * @param db_path Path to SQLite database file
     */
    explicit SQLiteCollisionStore(const std::string& db_path) : db_path_(db_path) {}
    
    /**
     * @brief Destructor - closes database and cleans up
     */
    ~SQLiteCollisionStore() {
        if (insert_collision_stmt_) sqlite3_finalize(insert_collision_stmt_);
        if (insert_test_run_stmt_) sqlite3_finalize(insert_test_run_stmt_);
        if (db_) sqlite3_close(db_);
    }
    
    /**
     * @brief Initialize the database and create tables
     * @return True on success
     */
    bool initialize() override {
        int rc = sqlite3_open(db_path_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            return false;
        }
        
        // Enable WAL mode for better concurrent access
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
        
        // Create tables
        const char* create_collisions_sql = R"(
            CREATE TABLE IF NOT EXISTS collisions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                hash_value INTEGER NOT NULL,
                input1 BLOB NOT NULL,
                input2 BLOB NOT NULL,
                input1_index INTEGER,
                input2_index INTEGER,
                timestamp INTEGER NOT NULL,
                algorithm TEXT NOT NULL,
                table_size INTEGER NOT NULL,
                run_id TEXT
            );
            CREATE INDEX IF NOT EXISTS idx_hash_value ON collisions(hash_value);
            CREATE INDEX IF NOT EXISTS idx_algorithm ON collisions(algorithm);
            CREATE INDEX IF NOT EXISTS idx_run_id ON collisions(run_id);
        )";
        
        const char* create_test_runs_sql = R"(
            CREATE TABLE IF NOT EXISTS test_runs (
                run_id TEXT PRIMARY KEY,
                algorithm TEXT NOT NULL,
                table_size INTEGER NOT NULL,
                num_hashes INTEGER NOT NULL,
                timestamp INTEGER NOT NULL,
                avalanche_score REAL,
                chi_squared REAL,
                collision_ratio REAL,
                actual_collisions INTEGER,
                expected_collisions REAL,
                throughput_mbs REAL,
                ns_per_hash REAL,
                metadata_json TEXT
            );
            CREATE INDEX IF NOT EXISTS idx_test_algorithm ON test_runs(algorithm);
            CREATE INDEX IF NOT EXISTS idx_test_timestamp ON test_runs(timestamp);
        )";
        
        rc = sqlite3_exec(db_, create_collisions_sql, nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) return false;
        
        rc = sqlite3_exec(db_, create_test_runs_sql, nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) return false;
        
        // Prepare statements
        const char* insert_collision_sql = R"(
            INSERT INTO collisions (hash_value, input1, input2, input1_index, input2_index, 
                                  timestamp, algorithm, table_size, run_id)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
        )";
        
        rc = sqlite3_prepare_v2(db_, insert_collision_sql, -1, &insert_collision_stmt_, nullptr);
        if (rc != SQLITE_OK) return false;
        
        const char* insert_test_run_sql = R"(
            INSERT OR REPLACE INTO test_runs 
            (run_id, algorithm, table_size, num_hashes, timestamp, avalanche_score,
             chi_squared, collision_ratio, actual_collisions, expected_collisions,
             throughput_mbs, ns_per_hash, metadata_json)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
        )";
        
        rc = sqlite3_prepare_v2(db_, insert_test_run_sql, -1, &insert_test_run_stmt_, nullptr);
        return rc == SQLITE_OK;
    }
    
    /**
     * @brief Store a single collision
     * @param record Collision to store
     * @return True on success
     */
    bool store_collision(const CollisionRecord& record) override {
        if (!insert_collision_stmt_) return false;
        
        sqlite3_reset(insert_collision_stmt_);
        sqlite3_bind_int64(insert_collision_stmt_, 1, record.hash_value);
        sqlite3_bind_blob(insert_collision_stmt_, 2, record.input1.data(), 
                         record.input1.size(), SQLITE_STATIC);
        sqlite3_bind_blob(insert_collision_stmt_, 3, record.input2.data(), 
                         record.input2.size(), SQLITE_STATIC);
        sqlite3_bind_int64(insert_collision_stmt_, 4, record.input1_index);
        sqlite3_bind_int64(insert_collision_stmt_, 5, record.input2_index);
        sqlite3_bind_int64(insert_collision_stmt_, 6, record.timestamp);
        sqlite3_bind_text(insert_collision_stmt_, 7, record.algorithm.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(insert_collision_stmt_, 8, record.table_size);
        sqlite3_bind_null(insert_collision_stmt_, 9); // run_id, can be set later
        
        return sqlite3_step(insert_collision_stmt_) == SQLITE_DONE;
    }
    
    /**
     * @brief Store multiple collisions efficiently
     * @param records Collisions to store
     * @return Number of records successfully stored
     */
    size_t store_collisions_batch(const std::vector<CollisionRecord>& records) override {
        size_t stored = 0;
        
        sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
        
        for (const auto& record : records) {
            if (store_collision(record)) {
                stored++;
            }
        }
        
        sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
        return stored;
    }
    
    /**
     * @brief Store test run with metrics
     * @param record Test run to store
     * @return True on success
     */
    bool store_test_run(const TestRunRecord& record) override {
        if (!insert_test_run_stmt_) return false;
        
        sqlite3_reset(insert_test_run_stmt_);
        sqlite3_bind_text(insert_test_run_stmt_, 1, record.run_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(insert_test_run_stmt_, 2, record.algorithm.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(insert_test_run_stmt_, 3, record.table_size);
        sqlite3_bind_int64(insert_test_run_stmt_, 4, record.num_hashes);
        sqlite3_bind_int64(insert_test_run_stmt_, 5, record.timestamp);
        sqlite3_bind_double(insert_test_run_stmt_, 6, record.avalanche_score);
        sqlite3_bind_double(insert_test_run_stmt_, 7, record.chi_squared);
        sqlite3_bind_double(insert_test_run_stmt_, 8, record.collision_ratio);
        sqlite3_bind_int64(insert_test_run_stmt_, 9, record.actual_collisions);
        sqlite3_bind_double(insert_test_run_stmt_, 10, record.expected_collisions);
        sqlite3_bind_double(insert_test_run_stmt_, 11, record.throughput_mbs);
        sqlite3_bind_double(insert_test_run_stmt_, 12, record.ns_per_hash);
        sqlite3_bind_text(insert_test_run_stmt_, 13, record.metadata_json.c_str(), -1, SQLITE_STATIC);
        
        return sqlite3_step(insert_test_run_stmt_) == SQLITE_DONE;
    }
    
    /**
     * @brief Query collisions by hash value
     * @param hash_value Hash to search for
     * @param algorithm Optional algorithm filter
     * @return Vector of collision records
     */
    std::vector<CollisionRecord> query_collisions(uint64_t hash_value, 
                                                  const std::string& algorithm = "") override {
        std::vector<CollisionRecord> results;
        
        std::string sql = "SELECT hash_value, input1, input2, input1_index, input2_index, "
                         "timestamp, algorithm, table_size FROM collisions WHERE hash_value = ?";
        if (!algorithm.empty()) {
            sql += " AND algorithm = ?";
        }
        sql += " LIMIT 1000;";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return results;
        }
        
        sqlite3_bind_int64(stmt, 1, hash_value);
        if (!algorithm.empty()) {
            sqlite3_bind_text(stmt, 2, algorithm.c_str(), -1, SQLITE_STATIC);
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CollisionRecord record;
            record.hash_value = sqlite3_column_int64(stmt, 0);
            
            const void* blob1 = sqlite3_column_blob(stmt, 1);
            int blob1_size = sqlite3_column_bytes(stmt, 1);
            record.input1.assign(static_cast<const uint8_t*>(blob1), 
                               static_cast<const uint8_t*>(blob1) + blob1_size);
            
            const void* blob2 = sqlite3_column_blob(stmt, 2);
            int blob2_size = sqlite3_column_bytes(stmt, 2);
            record.input2.assign(static_cast<const uint8_t*>(blob2), 
                               static_cast<const uint8_t*>(blob2) + blob2_size);
            
            record.input1_index = sqlite3_column_int64(stmt, 3);
            record.input2_index = sqlite3_column_int64(stmt, 4);
            record.timestamp = sqlite3_column_int64(stmt, 5);
            record.algorithm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            record.table_size = sqlite3_column_int64(stmt, 7);
            
            results.push_back(record);
        }
        
        sqlite3_finalize(stmt);
        return results;
    }
    
    /**
     * @brief Get test run by ID
     * @param run_id Test run identifier
     * @return Test run record if found
     */
    std::optional<TestRunRecord> get_test_run(const std::string& run_id) override {
        const char* sql = "SELECT * FROM test_runs WHERE run_id = ? LIMIT 1;";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return std::nullopt;
        }
        
        sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            TestRunRecord record;
            record.run_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            record.algorithm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            record.table_size = sqlite3_column_int64(stmt, 2);
            record.num_hashes = sqlite3_column_int64(stmt, 3);
            record.timestamp = sqlite3_column_int64(stmt, 4);
            record.avalanche_score = sqlite3_column_double(stmt, 5);
            record.chi_squared = sqlite3_column_double(stmt, 6);
            record.collision_ratio = sqlite3_column_double(stmt, 7);
            record.actual_collisions = sqlite3_column_int64(stmt, 8);
            record.expected_collisions = sqlite3_column_double(stmt, 9);
            record.throughput_mbs = sqlite3_column_double(stmt, 10);
            record.ns_per_hash = sqlite3_column_double(stmt, 11);
            if (sqlite3_column_text(stmt, 12)) {
                record.metadata_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            }
            
            sqlite3_finalize(stmt);
            return record;
        }
        
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    
    /**
     * @brief Get test runs for an algorithm
     * @param algorithm Algorithm name
     * @param limit Maximum results
     * @return Vector of test run records
     */
    std::vector<TestRunRecord> get_test_runs(const std::string& algorithm, 
                                            size_t limit = 100) override {
        std::vector<TestRunRecord> results;
        
        const char* sql = "SELECT * FROM test_runs WHERE algorithm = ? "
                         "ORDER BY timestamp DESC LIMIT ?;";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return results;
        }
        
        sqlite3_bind_text(stmt, 1, algorithm.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, limit);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            TestRunRecord record;
            record.run_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            record.algorithm = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            record.table_size = sqlite3_column_int64(stmt, 2);
            record.num_hashes = sqlite3_column_int64(stmt, 3);
            record.timestamp = sqlite3_column_int64(stmt, 4);
            record.avalanche_score = sqlite3_column_double(stmt, 5);
            record.chi_squared = sqlite3_column_double(stmt, 6);
            record.collision_ratio = sqlite3_column_double(stmt, 7);
            record.actual_collisions = sqlite3_column_int64(stmt, 8);
            record.expected_collisions = sqlite3_column_double(stmt, 9);
            record.throughput_mbs = sqlite3_column_double(stmt, 10);
            record.ns_per_hash = sqlite3_column_double(stmt, 11);
            if (sqlite3_column_text(stmt, 12)) {
                record.metadata_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            }
            
            results.push_back(record);
        }
        
        sqlite3_finalize(stmt);
        return results;
    }
};

/**
 * @brief Generate a unique run ID
 * @param algorithm Algorithm name
 * @return Unique run identifier
 */
inline std::string generate_run_id(const std::string& algorithm) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << algorithm << "_" << timestamp << "_" << (rand() % 10000);
    return ss.str();
}

} // namespace goldenhash::tests