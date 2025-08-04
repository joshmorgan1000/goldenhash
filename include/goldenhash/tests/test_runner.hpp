#pragma once

#include "common.hpp"
#include "metrics.hpp"
#include "collision_store.hpp"
#include "hash64_analyzer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <random>

namespace goldenhash::tests {

/**
 * @brief Test runner that executes hash tests and collects metrics
 */
class TestRunner {
private:
    std::vector<MapShard*> shards_;
    TestData* test_data_;
    GoldenHash& hasher_;
    ComparisonResult result_;
    size_t number_of_important_bits_{0};
    std::unique_ptr<std::thread> performance_thread;
    std::unique_ptr<std::thread> metrics_thread;
    
    // Metrics collectors
    std::unique_ptr<AvalancheAnalyzer> avalanche_analyzer_;
    std::unique_ptr<ChiSquaredCalculator> chi_squared_calc_;
    std::unique_ptr<CollisionAnalyzer> collision_analyzer_;
    std::unique_ptr<CollisionStore> collision_store_;
    std::unique_ptr<Hash64Analyzer> hash64_analyzer_;
    
    bool collect_metrics_ = false;
    bool store_collisions_ = false;
    bool analyze_64bit_ = false;
    
public:
    std::atomic<bool> performance_benchmark_complete_{false};
    std::atomic<bool> metrics_collection_complete_{false};

    TestRunner(std::vector<MapShard*> shards, TestData* test_data, GoldenHash& hasher, std::string algorithm, size_t table_size) : shards_(shards), test_data_(test_data), hasher_(hasher) {
        if (shards_.size() != 64) {
            throw std::runtime_error("These tests require exactly 64 shards");
        }
        result_.algorithm = algorithm;
        result_.table_size = table_size;
        result_.ns_per_hash = 0.0;
        result_.throughput_mbs = 0.0;
        result_.total_time_ms = 0.0;
        if (algorithm == "goldenhash") {
            result_.prime_high = hasher.get_prime_high();
            result_.prime_low = hasher.get_prime_low();
            result_.working_modulus = hasher.get_working_mod();
            result_.factors = hasher.get_factors();
        } else {
            result_.prime_high = 0;
            result_.prime_low = 0;
            result_.working_modulus = 0;
            result_.factors.clear();
        }
        // Calculate the number of important bits based on the table size
        number_of_important_bits_ = static_cast<size_t>(std::ceil(std::log2(table_size)));
        if (number_of_important_bits_ > 64) {
            throw std::runtime_error("Table size is too large, cannot handle more than 64 important bits");
        }
    }

    ComparisonResult get_result() const {
        if (!performance_benchmark_complete_.load()) {
            throw std::runtime_error("Performance benchmark has not completed yet");
        }
        return result_;
    }

    /**
     * @brief Enable metrics collection
     * @param chi_squared_buckets Number of buckets for chi-squared test
     * @param collision_db_path Optional path to collision database
     * @param analyze_64bit Enable 64-bit hash analysis (no modulo)
     */
    void enable_metrics(size_t chi_squared_buckets = 256, const std::string& collision_db_path = "", 
                       bool analyze_64bit = false) {
        collect_metrics_ = true;
        
        // Initialize metrics collectors
        avalanche_analyzer_ = std::make_unique<AvalancheAnalyzer>(number_of_important_bits_);
        chi_squared_calc_ = std::make_unique<ChiSquaredCalculator>(chi_squared_buckets);
        collision_analyzer_ = std::make_unique<CollisionAnalyzer>(result_.table_size);
        
        if (!collision_db_path.empty()) {
            store_collisions_ = true;
            collision_store_ = std::make_unique<SQLiteCollisionStore>(collision_db_path);
            collision_store_->initialize();
        }
        
        analyze_64bit_ = analyze_64bit;
        if (analyze_64bit) {
            hash64_analyzer_ = std::make_unique<Hash64Analyzer>(collision_db_path, test_data_->size());
        }
    }
    
    /**
     * @brief Run metrics collection (separate thread)
     */
    void run_metrics_collection() {
        if (!collect_metrics_ || !test_data_) return;
        
        metrics_thread = std::make_unique<std::thread>([this]() {
            size_t num_tests = test_data_->size();
            std::vector<CollisionRecord> collision_batch;
            
            // Collect metrics on test data
            for (size_t i = 0; i < num_tests; ++i) {
                std::string data_str = test_data_->get_test(i);
                std::vector<uint8_t> data(data_str.begin(), data_str.end());
                uint64_t hash = compute_hash(result_.algorithm, data.data(), data.size(), 
                                           result_.table_size, hasher_);
                
                // Chi-squared distribution
                chi_squared_calc_->add_sample(hash, result_.table_size);
                
                // Collision analysis
                size_t prev_collisions = collision_analyzer_->get_actual_collisions();
                collision_analyzer_->add_hash(hash, i);
                
                // 64-bit hash analysis (if enabled)
                if (analyze_64bit_) {
                    // Get the full 64-bit hash without modulo
                    uint64_t full_hash = compute_hash(result_.algorithm, data.data(), data.size(), 
                                                    UINT64_MAX, hasher_);  // No modulo
                    hash64_analyzer_->add_hash(full_hash, data.data(), data.size());
                }
                
                // If a new collision was detected, store it
                if (store_collisions_ && collision_analyzer_->get_actual_collisions() > prev_collisions) {
                    // Find the previous occurrence
                    for (size_t j = 0; j < i; ++j) {
                        std::string prev_data_str = test_data_->get_test(j);
                        std::vector<uint8_t> prev_data(prev_data_str.begin(), prev_data_str.end());
                        uint64_t prev_hash = compute_hash(result_.algorithm, prev_data.data(), 
                                                        prev_data.size(), result_.table_size, hasher_);
                        if (prev_hash == hash) {
                            CollisionRecord record;
                            record.hash_value = hash;
                            record.input1 = prev_data;
                            record.input2 = data;
                            record.input1_index = j;
                            record.input2_index = i;
                            record.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                            record.algorithm = result_.algorithm;
                            record.table_size = result_.table_size;
                            collision_batch.push_back(record);
                            break;
                        }
                    }
                }
                
                // Avalanche effect - test with bit flips
                if (i % 100 == 0 && data.size() > 0) {  // Sample every 100th entry
                    for (size_t byte_idx = 0; byte_idx < data.size(); ++byte_idx) {
                        for (int bit = 0; bit < 8; ++bit) {
                            // Flip one bit
                            data[byte_idx] ^= (1 << bit);
                            uint64_t hash_flipped = compute_hash(result_.algorithm, data.data(), 
                                                               data.size(), result_.table_size, hasher_);
                            avalanche_analyzer_->add_sample(hash, hash_flipped);
                            // Flip back
                            data[byte_idx] ^= (1 << bit);
                        }
                    }
                }
            }
            
            // Store collision batch
            if (store_collisions_ && !collision_batch.empty()) {
                collision_store_->store_collisions_batch(collision_batch);
            }
            
            // Update result with metrics
            result_.avalanche_score = avalanche_analyzer_->get_avalanche_score();
            result_.avalanche_bias = avalanche_analyzer_->get_avalanche_bias();
            result_.chi_squared = chi_squared_calc_->get_chi_squared();
            result_.uniformity_score = chi_squared_calc_->get_uniformity_score();
            result_.collision_ratio = collision_analyzer_->get_collision_ratio();
            result_.actual_collisions = collision_analyzer_->get_actual_collisions();
            result_.expected_collisions = collision_analyzer_->get_expected_collisions();
            result_.load_factor = collision_analyzer_->get_load_factor();
            result_.metrics_collected = true;
            
            // Store test run if we have a collision store
            if (store_collisions_) {
                TestRunRecord run_record;
                run_record.run_id = generate_run_id(result_.algorithm);
                run_record.algorithm = result_.algorithm;
                run_record.table_size = result_.table_size;
                run_record.num_hashes = num_tests;
                run_record.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
                run_record.avalanche_score = result_.avalanche_score;
                run_record.chi_squared = result_.chi_squared;
                run_record.collision_ratio = result_.collision_ratio;
                run_record.actual_collisions = result_.actual_collisions;
                run_record.expected_collisions = result_.expected_collisions;
                run_record.throughput_mbs = result_.throughput_mbs;
                run_record.ns_per_hash = result_.ns_per_hash;
                
                collision_store_->store_test_run(run_record);
                
                // Also save 64-bit analysis if enabled
                if (analyze_64bit_) {
                    std::string metadata = "{\"table_size\": " + std::to_string(result_.table_size) + 
                                         ", \"analysis\": \"" + hash64_analyzer_->get_statistics() + "\"}";
                    hash64_analyzer_->save_results(result_.algorithm, metadata);
                }
            }
            
            metrics_collection_complete_.store(true);
            metrics_collection_complete_.notify_all();
        });
        metrics_thread->detach();
    }
    
    /**
     * @brief Runs the performance benchmark (single-threaded)
     */
    void run_performance_benchmark() {
        if (!test_data_) {
            throw std::runtime_error("Test data is not initialized");
        }
        performance_thread = std::make_unique<std::thread>([this]() {
            size_t total_bytes = 0;
            size_t num_hashes = 0;
            size_t num_tests = std::min<size_t>(1000000, test_data_->size());
            size_t warmup = std::min<size_t>(1000, num_tests);
            // Warm up
            for (size_t i = 0; i < warmup; ++i) {
                std::string data_str = test_data_->get_test(i);
                std::vector<uint8_t> data(data_str.begin(), data_str.end());
                compute_hash(result_.algorithm, data.data(), data.size(), result_.table_size, hasher_);
            }
            // Benchmark
            auto start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < num_tests; ++i) {
                std::string data_str = test_data_->get_test(i);
                total_bytes += data_str.size();
                std::vector<uint8_t> data(data_str.begin(), data_str.end());
                compute_hash(result_.algorithm, data.data(), data.size(), result_.table_size, hasher_);
                num_hashes++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            result_.ns_per_hash = static_cast<double>(duration) / num_hashes;
            result_.throughput_mbs = (total_bytes / (1024.0 * 1024.0)) / (duration / 1e9);
            result_.total_time_ms = static_cast<double>(duration) / 1e6;
            performance_benchmark_complete_.store(true);
            performance_benchmark_complete_.notify_all();
        });
        performance_thread->detach();
    }
    
};

} // namespace goldenhash::tests