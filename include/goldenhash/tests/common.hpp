#pragma once

#include "goldenhash.hpp"
#include "hash_algorithms.hpp"
#include "map_shard.hpp"
#include "sqlite_shard.hpp"
#include "memory_utils.hpp"
#include "sqlite_test_data.hpp"
#include "test_data.hpp"
#include <string>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>

namespace goldenhash::tests {

/**
 * @brief Structure to store algorithm comparison results
 */
struct ComparisonResult {
    std::string algorithm;
    size_t table_size;
    double throughput_mbs;
    double ns_per_hash;
    uint64_t unique_hashes;
    uint64_t total_collisions;
    double collision_ratio;
    uint64_t max_bucket_load;
    double chi_square;
    double avalanche_score;
    double total_time_ms;
    uint64_t prime_high;
    uint64_t prime_low;
    uint64_t working_modulus;
    std::vector<uint64_t> factors;
};

/**
 * @brief Test data generator that creates test data in parallel
 */
class TestDataGenerator {
public:
    static std::vector<std::unique_ptr<TestData>> generate(
        uint64_t num_iterations,
        int num_threads,
        bool use_sqlite,
        bool json_output) {
        
        if (!json_output) {
            std::cout << "Generating test data using " << num_threads << " threads...\n";
        }
        
        std::vector<std::unique_ptr<TestData>> thread_test_data;
        thread_test_data.reserve(num_threads);
        
        for (int t = 0; t < num_threads; ++t) {
            if (use_sqlite) {
                thread_test_data.push_back(std::make_unique<SQLiteTestData>(":memory:"));
            } else {
                thread_test_data.push_back(std::make_unique<InMemoryTestData>());
            }
        }
        
        auto gen_start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> gen_threads;
        size_t items_per_thread = num_iterations / num_threads;
        size_t remainder = num_iterations % num_threads;
        
        std::atomic<size_t> threads_completed(0);
        
        // For progress tracking
        std::atomic<size_t> total_items_generated(0);
        auto gen_start_time = std::chrono::high_resolution_clock::now();
        
        for (int t = 0; t < num_threads; ++t) {
            size_t start_idx = t * items_per_thread + std::min(static_cast<size_t>(t), remainder);
            size_t end_idx = start_idx + items_per_thread + (t < static_cast<int>(remainder) ? 1 : 0);
            
            gen_threads.emplace_back([&, t, start_idx, end_idx]() {
                create_test_data(thread_test_data[t].get(), start_idx, end_idx, &total_items_generated);
                threads_completed.fetch_add(1);
            });
        }
        
        // Progress bar for test data generation
        if (!json_output && num_iterations > 100000) {  // Only show progress for large datasets
            const int progress_bar_width = 50;
            size_t last_progress_update = 0;
            
            while (threads_completed.load() < static_cast<size_t>(num_threads)) {
                size_t generated = total_items_generated.load(std::memory_order_relaxed);
                
                // Only update if progress changed significantly
                if (generated - last_progress_update < num_iterations / 100 && 
                    threads_completed.load() < static_cast<size_t>(num_threads)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                last_progress_update = generated;
                
                double progress = static_cast<double>(generated) / num_iterations;
                
                // Time estimation
                auto elapsed = std::chrono::high_resolution_clock::now() - gen_start_time;
                double elapsed_sec = std::chrono::duration<double>(elapsed).count();
                double estimated_total = elapsed_sec / progress;
                double estimated_remaining = estimated_total - elapsed_sec;
                
                // Progress bar
                std::cout << "\r  [";
                int filled = static_cast<int>(progress * progress_bar_width);
                for (int i = 0; i < progress_bar_width; ++i) {
                    if (i < filled) std::cout << "=";
                    else if (i == filled) std::cout << ">";
                    else std::cout << " ";
                }
                std::cout << "] " << std::fixed << std::setprecision(1) 
                          << (progress * 100) << "% "
                          << "(" << (generated / 1000000) << "M/" << (num_iterations / 1000000) << "M) "
                          << "ETA: " << static_cast<int>(estimated_remaining) << "s" << std::flush;
            }
            std::cout << "\r  [" << std::string(progress_bar_width, '=') << "] 100.0% "
                      << "(" << (num_iterations / 1000000) << "M/" << (num_iterations / 1000000) << "M) "
                      << "Done!                    \n";
        }
        
        for (auto& t : gen_threads) {
            t.join();
        }
        
        auto gen_end = std::chrono::high_resolution_clock::now();
        auto gen_duration = std::chrono::duration_cast<std::chrono::milliseconds>(gen_end - gen_start);
        
        if (!json_output) {
            std::cout << "Test data generated in " << gen_duration.count() << " ms\n";
        }
        
        return thread_test_data;
    }
};

} // namespace goldenhash
