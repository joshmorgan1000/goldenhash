#pragma once

#include "common.hpp"
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
    TestData* test_data_;
    std::vector<MapShard*> shards_;
    GoldenHash& hasher_;
    ComparisonResult result_;
    size_t number_of_important_bits_{0};
    std::unique_ptr<std::thread> performance_thread;
    std::unique_ptr<std::thread> collision_thread;
    
public:
    std::atomic<bool> performance_benchmark_complete_{false};
    std::atomic<bool> collision_test_complete_{false};
    std::atomic<uint64_t> hashes_{0};
    std::atomic<uint64_t> unique_{0};
    std::atomic<uint64_t> collisions_{0};

    TestRunner(const std::vector<MapShard*>& shards, TestData* test_data, GoldenHash& hasher, std::string algorithm, size_t table_size) : shards_(shards), test_data_(test_data), hasher_(hasher) {
        if (shards_.size() != 64) {
            throw std::runtime_error("These tests require exactly 64 shards");
        }
        result_.algorithm = algorithm;
        result_.table_size = table_size;
        result_.unique_hashes = 0;
        result_.total_collisions = 0;
        result_.collision_ratio = 0.0;
        result_.max_bucket_load = 0;
        result_.chi_square = 0.0;
        result_.avalanche_score = 0.0;
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
        if (!performance_benchmark_complete_.load() || !collision_test_complete_.load()) {
            throw std::runtime_error("Tests have not completed yet");
        }
        return result_;
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
                uint64_t hash = compute_hash(result_.algorithm, data.data(), data.size(), result_.table_size, hasher_);
            }
            // Benchmark
            auto start = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < num_tests; ++i) {
                std::string data_str = test_data_->get_test(i);
                total_bytes += data_str.size();
                std::vector<uint8_t> data(data_str.begin(), data_str.end());
                uint64_t hash = compute_hash(result_.algorithm, data.data(), data.size(), result_.table_size, hasher_);
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
    
    /**
     * @brief Runs the assigned tests and collects metrics in a separate thread
     */
    void run_collision_test() {
        if (!test_data_) {
            throw std::runtime_error("Test data is not initialized");
        }
        collision_thread = std::make_unique<std::thread>([this]() {
            size_t total_items = test_data_->size();
            size_t max_bucket_load = 0;
            double chi_square = 0.0;
            double avalanche_score = 0.0;

            for (size_t i = 0; i < total_items; ++i) {
                std::string data_str = test_data_->get_test(i);
                std::vector<uint8_t> data(data_str.begin(), data_str.end());
                uint64_t hash = compute_hash(result_.algorithm, data.data(), data.size(), result_.table_size, hasher_);
                size_t shard_index = hash & 0x3F;  // We require exactly 64 shards
                bool collision = shards_[shard_index]->process_hash(hash);
                
                // Debug: check if we're getting duplicate hashes
                if (i < 10 && result_.algorithm == "goldenhash") {
                    std::cout << "Hash[" << i << "]: " << hash << " (data: " << data_str.substr(0, 20) << "...)\n";
                }
                
                // Update avalanche score
                if ((i & 0x3FF) == 0 && !data.empty()) {
                    std::vector<uint8_t> modified_data = data;
                    // Modify one random bit in the input data
                    size_t byte_index = rand() % modified_data.size();
                    size_t bit_offset = rand() % 8;
                    modified_data[byte_index] ^= (1 << bit_offset);
                    
                    uint64_t original_hash = hash;
                    uint64_t modified_hash = compute_hash(result_.algorithm, modified_data.data(), modified_data.size(), result_.table_size, hasher_);
                    
                    // Only check the bits that matter for this table size
                    uint64_t mask = (1ULL << number_of_important_bits_) - 1;
                    uint64_t diff = (original_hash ^ modified_hash) & mask;
                    int bits_changed = __builtin_popcountll(diff);
                    avalanche_score += bits_changed;
                }
                if (collision) {
                    collisions_.fetch_add(1, std::memory_order_relaxed);
                } else {
                    unique_.fetch_add(1, std::memory_order_relaxed);
                }
                hashes_.fetch_add(1, std::memory_order_relaxed);
            }
            
            result_.total_collisions = collisions_.load(std::memory_order_relaxed);
            result_.unique_hashes = unique_.load(std::memory_order_relaxed);
            result_.max_bucket_load = max_bucket_load;
            // Calculate average avalanche score (bits changed / important bits)
            size_t num_avalanche_tests = (total_items + 0x3FF) / 0x400;  // Number of avalanche tests performed
            if (num_avalanche_tests > 0) {
                result_.avalanche_score = (avalanche_score / num_avalanche_tests) / number_of_important_bits_;
            } else {
                result_.avalanche_score = 0.0;
            }
            collision_test_complete_.store(true);
            collision_test_complete_.notify_all();
        });
        collision_thread->detach();
    }
};

} // namespace goldenhash::tests