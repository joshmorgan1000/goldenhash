/**
 * Copyright 2025 Josh Morgan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file main.cpp
 * @brief Main test program for GoldenHash library
 * @author Josh Morgan
 * @date 2025
 * 
 * This program runs comprehensive tests on the GoldenHash implementations,
 * including FloatHash and metrics analysis.
 */

#include <goldenhash.hpp>
#include <goldenhash/tests/common.hpp>
#include <goldenhash/tests/test_runner.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <getopt.h>

using namespace goldenhash;
using namespace goldenhash::tests;

void display_comparison_table(const std::vector<ComparisonResult>& results) {
    if (results.empty()) return;
    
    std::cout << "\n=== HASH ALGORITHM COMPARISON ===\n\n";
    
    // Header
    std::cout << std::left 
              << std::setw(10) << "Algorithm"
              << " | " << std::right
              << std::setw(11) << "Throughput"
              << " | " << std::setw(8) << "ns/hash"
              << " | " << std::setw(7) << "Colls"
              << " | " << std::setw(15) << "Collision Ratio"
              << " | " << std::setw(4) << "Max"
              << " | " << std::setw(15) << "Chi-Square"
              << " | " << std::setw(15) << "Avalanche"
              << " | " << std::setw(6) << "Ms"
              << "\n";
    
    std::cout << std::string(10, '-') << "-+-" 
              << std::string(11, '-') << "-+-"
              << std::string(8, '-') << "-+-"
              << std::string(7, '-') << "-+-"
              << std::string(15, '-') << "-+-"
              << std::string(4, '-') << "-+-"
              << std::string(15, '-') << "-+-"
              << std::string(15, '-') << "-+-"
              << std::string(6, '-') << "\n";
    
    // Results
    for (const auto& r : results) {
        std::ostringstream throughput_str;
        throughput_str << std::fixed << std::setprecision(1) << r.throughput_mbs << " MB/s";
        
        std::cout << std::left << std::setw(10) << r.algorithm
                  << " | " << std::right
                  << std::setw(11) << throughput_str.str()
                  << " | " << std::setw(8) << std::fixed << std::setprecision(1) << r.ns_per_hash
                  << " | " << std::setw(7) << std::noshowpoint << std::setprecision(0) << r.total_collisions
                  << " | " << std::setw(15) << std::fixed << std::setprecision(9) 
                  << r.collision_ratio
                  << " | " << std::setw(4) << std::noshowpoint << std::setprecision(0) << r.max_bucket_load
                  << " | " << std::setw(15) << std::fixed << std::setprecision(9) << r.chi_square
                  << " | " << std::setw(15) << std::fixed << std::setprecision(9) << r.avalanche_score
                  << " | " << std::setw(6) << std::noshowpoint << std::setprecision(0) << r.total_time_ms
                  << "\n";
    }
    
    std::cout << "\n";
}

void output_json_results(const ComparisonResult& result, uint64_t table_size, 
                               uint64_t num_iterations, GoldenHash& hasher) {
    // Generate test vectors
    std::vector<std::pair<std::string, uint64_t>> test_hashes;
    const std::vector<std::string> test_strings = {
        "", "Hello, World!", "1234567890", "a", "abc",
        "The quick brown fox jumps over the lazy dog"
    };
    for (const auto& str : test_strings) {
        std::vector<uint8_t> data(str.begin(), str.end());
        uint64_t hash = hasher.hash(data.data(), data.size());
        test_hashes.push_back({str, hash});
    }
    
    // Calculate expected collisions
    double expected_collisions = num_iterations - table_size * (1.0 - std::pow(1.0 - 1.0/table_size, num_iterations));
    
    // JSON output
    std::cout << "{\n";
    std::cout << "  \"table_size\": " << table_size << ",\n";
    std::cout << "  \"unique_hashes\": " << result.unique_hashes << ",\n";
    std::cout << "  \"distribution_uniformity\": " << std::sqrt(result.chi_square / table_size) << ",\n";
    std::cout << "  \"total_collisions\": " << result.total_collisions << ",\n";
    std::cout << "  \"expected_collisions\": " << expected_collisions << ",\n";
    std::cout << "  \"collision_ratio\": " << result.collision_ratio << ",\n";
    std::cout << "  \"max_bucket_load\": " << result.max_bucket_load << ",\n";
    std::cout << "  \"avalanche_score\": " << result.avalanche_score << ",\n";
    std::cout << "  \"chi_square\": " << result.chi_square << ",\n";
    std::cout << "  \"prime_high\": " << hasher.get_prime_high() << ",\n";
    std::cout << "  \"prime_low\": " << hasher.get_prime_low() << ",\n";
    std::cout << "  \"working_modulus\": " << hasher.get_working_mod() << ",\n";
    std::cout << "  \"test_vectors\": {\n";
    for (size_t i = 0; i < test_hashes.size(); i++) {
        std::cout << "    \"" << test_hashes[i].first << "\": " << test_hashes[i].second;
        if (i < test_hashes.size() - 1) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "  },\n";
    std::cout << "  \"factors\": \"";
    for (size_t i = 0; i < hasher.get_factors().size(); i++) {
        std::cout << hasher.get_factors()[i];
        if (i < hasher.get_factors().size() - 1) std::cout << ", ";
    }
    std::cout << "\",\n";
    std::cout << "  \"performance_ns_per_hash\": " << result.ns_per_hash << "\n";
    std::cout << "}\n";
}

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " <table_size> <iterations> [options]\n"
              << "\nOptions:\n"
              << "  --threads <n>      Number of threads (default: hardware concurrency)\n"
              << "  --force-sqlite     Force SQLite storage for all tests\n"
              << "  --compare          Compare all hash algorithms\n"
              << "  --algorithm <name> Test specific algorithm (goldenhash, xxhash64, sha256, aes-cmac)\n"
              << "  --json             Output results in JSON format\n"
              << "  --help             Show this help message\n";
}

int main(int argc, char* argv[]) {
    // Set a fixed random seed for reproducibility
    srand(42);
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    uint64_t table_size = std::stoull(argv[1]);
    uint64_t num_iterations = std::stoull(argv[2]);
    
    int num_threads = std::thread::hardware_concurrency();
    bool force_sqlite = false;
    bool compare_mode = false;
    bool json_output = false;
    std::string specific_algorithm;
    
    // Parse options
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"force-sqlite", no_argument, 0, 's'},
        {"compare", no_argument, 0, 'c'},
        {"algorithm", required_argument, 0, 'a'},
        {"json", no_argument, 0, 'j'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "t:sca:jh", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                num_threads = std::stoi(optarg);
                break;
            case 's':
                force_sqlite = true;
                break;
            case 'c':
                compare_mode = true;
                break;
            case 'a':
                specific_algorithm = optarg;
                break;
            case 'j':
                json_output = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Determine whether to use SQLite based on memory requirements
    bool use_sqlite = force_sqlite;
    if (!use_sqlite) {
        uint64_t available_memory = get_available_memory();
        uint64_t required_memory = table_size * 16; // Rough estimate
        if (required_memory > available_memory * 0.8) {
            if (!json_output) {
                std::cout << "Switching to SQLite storage (required: " 
                          << (required_memory / (1024*1024)) << " MB, available: "
                          << (available_memory / (1024*1024)) << " MB)\n";
            }
            use_sqlite = true;
        }
    }
    
    if (!json_output) {
        std::cout << "Testing with table size: " << table_size 
                  << ", iterations: " << num_iterations
                  << ", threads: " << num_threads
                  << ", storage: " << (use_sqlite ? "SQLite" : "Memory") << "\n\n";
    }

    // Generate the test data, split into the number of threads so each thread has its own data it is responsible for
    std::vector<std::unique_ptr<TestData>> test_data = TestDataGenerator::generate(num_iterations, num_threads, use_sqlite, json_output);

    // Calculate the expected number of collisions based on the birthday paradox
    // This gives us the expected number of items that are not unique
    double expected_collisions;
    double ratio = static_cast<double>(num_iterations) / static_cast<double>(table_size);
    if (ratio < 0.1) {
        // For small n/m ratios, use the approximation to avoid precision loss
        expected_collisions = (static_cast<double>(num_iterations) * (num_iterations - 1)) / (2.0 * table_size);
    } else {
        // For larger ratios, use the full formula
        expected_collisions = num_iterations - table_size * (1.0 - std::pow(1.0 - 1.0/table_size, num_iterations));
    }
    
    if (!json_output) {
        std::cout << "Expected items that are not unique: " << expected_collisions << "\n";
    }
    
    // Lambda to run test for a specific algorithm
    auto run_algorithm_test = [&](const std::string& algo) -> ComparisonResult {
        // Create 64 shards
        std::vector<MapShard*> shards;
        shards.reserve(64);
        
        for (int i = 0; i < 64; ++i) {
            if (use_sqlite) {
                std::string filename = "shard_" + std::to_string(i) + ".db";
                shards.push_back(new SQLiteShard(filename, i, 0, UINT64_MAX));
            } else {
                shards.push_back(new HashMapShard());
            }
        }

        std::vector<std::unique_ptr<TestRunner>> runners;
        runners.reserve(num_threads);
        GoldenHash hasher(table_size);
        for (int i = 0; i < num_threads; ++i) {
            runners.emplace_back(std::make_unique<TestRunner>(shards, test_data[i].get(), hasher, algo, table_size));
        }

        // Run performance tests first
        if (!json_output) {
            std::cout << "Running performance benchmarks...\n";
        }
        for (auto& runner : runners) {
            runner->run_performance_benchmark();
        }

        // Wait for all performance benchmarks to complete
        for (auto& runner : runners) {
            runner->performance_benchmark_complete_.wait(false);
        }
        
        if (!json_output) {
            std::cout << "Running collision tests...\n";
        }
        // Run collision tests
        for (auto& runner : runners) {
            runner->run_collision_test();
        }

        if (json_output) {
            // Wait for all collision tests to complete
            for (auto& runner : runners) {
                runner->collision_test_complete_.wait(false);
            }
        } else {

            // Periodically check the progress of collision tests and print status
            auto start_time = std::chrono::high_resolution_clock::now();
            size_t completed = 0;
            const int progress_bar_width = 50;
            
            while (completed < num_iterations) {
                completed = 0;
                size_t collisions = 0;
                for (const auto& runner : runners) {
                    completed += runner->hashes_.load();
                    collisions += runner->collisions_.load();
                }
                
                // Non-linear time estimation based on collision probability
                auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
                double elapsed_sec = std::chrono::duration<double>(elapsed).count();
                double progress = static_cast<double>(completed) / num_iterations;
                
                // Estimate time remaining (collision detection gets slower as more hashes are stored)
                double collision_probability = 1.0 - std::exp(-static_cast<double>(completed) / table_size);
                double speed_factor = 1.0 + collision_probability * 0.5; // Slowdown factor
                double estimated_total = elapsed_sec / progress * speed_factor;
                double estimated_remaining = estimated_total - elapsed_sec;
                
                // Calculate the current birthday paradox based on the number of completed hashes
                double current_expected_collisions;
                double current_ratio = static_cast<double>(completed) / static_cast<double>(table_size);
                if (current_ratio < 0.1) {
                    current_expected_collisions = (static_cast<double>(completed) * (completed - 1)) / (2.0 * table_size);
                } else {
                    current_expected_collisions = completed - table_size * (1.0 - std::pow(1.0 - 1.0/table_size, completed));
                }
                // Calculate the ratio based on the number of collisions and the birthday paradox
                double current_collision_ratio = current_expected_collisions > 0 ? collisions / current_expected_collisions : 0;
                
                // Progress bar
                std::cout << "\r[";
                int filled = static_cast<int>(progress * progress_bar_width);
                for (int i = 0; i < progress_bar_width; ++i) {
                    if (i < filled) std::cout << "=";
                    else if (i == filled) std::cout << ">";
                    else std::cout << " ";
                }
                std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100) << "% "
                          << "ETA: " << static_cast<int>(estimated_remaining) << "s "
                          << "Ratio: " << std::setprecision(6) << (current_collision_ratio * 100) << "%" << std::flush;
                
                if (completed >= num_iterations) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            std::cout << std::endl;
            
            // Wait for all collision tests to complete
            for (auto& runner : runners) {
                runner->collision_test_complete_.wait(false);
            }
        }
        
        // Aggregate results from all runners
        ComparisonResult result;
        result.algorithm = algo;
        result.table_size = table_size;
        result.ns_per_hash = 0;
        result.throughput_mbs = 0;
        result.total_time_ms = 0;
        result.unique_hashes = 0;
        result.total_collisions = 0;
        result.max_bucket_load = 0;
        result.chi_square = 0;
        result.avalanche_score = 0;
        result.collision_ratio = 0;
        result.prime_high = 0;
        result.prime_low = 0;
        result.working_modulus = 0;
        result.factors.clear();
        
        // Collect performance metrics (average across threads)
        for (const auto& runner : runners) {
            auto runner_result = runner->get_result();
            result.ns_per_hash += runner_result.ns_per_hash;
            result.throughput_mbs += runner_result.throughput_mbs;
            result.total_time_ms = std::max(result.total_time_ms, runner_result.total_time_ms);
            result.avalanche_score += runner_result.avalanche_score;
        }
        result.ns_per_hash /= runners.size();
        result.avalanche_score /= runners.size();
        
        // Collect metrics from runners (they already track unique/collisions)
        uint64_t total_unique = 0;
        uint64_t total_collisions = 0;
        for (const auto& runner : runners) {
            total_unique += runner->unique_.load();
            total_collisions += runner->collisions_.load();
        }
        
        // Collect shard metrics for max load and distribution
        uint64_t max_bucket_load = 0;
        std::vector<uint64_t> shard_hash_counts(64, 0);
        
        for (size_t i = 0; i < 64; ++i) {
            uint64_t shard_max = shards[i]->get_max_count();
            if (shard_max > max_bucket_load) {
                max_bucket_load = shard_max;
            }
            // Total hashes processed by this shard
            shard_hash_counts[i] = shards[i]->get_unique() + shards[i]->get_collisions();
        }
        
        if (!json_output && algo == "goldenhash" && max_bucket_load > 100) {
            std::cout << "WARNING: Max bucket load is " << max_bucket_load 
                      << " which seems too high for " << num_iterations << " items\n";
        }
        
        
        result.unique_hashes = total_unique;
        result.max_bucket_load = max_bucket_load;
        
        // The correct number of collisions is simply: items processed - unique items
        // Don't use the shard collision counters as they count collision events, not actual collisions
        int64_t actual_collisions = static_cast<int64_t>(num_iterations) - static_cast<int64_t>(total_unique);
        if (actual_collisions < 0) {
            actual_collisions = 0;  // Shouldn't happen, but protect against underflow
        }
        result.total_collisions = actual_collisions;
        result.collision_ratio = expected_collisions > 0 ? static_cast<double>(actual_collisions) / expected_collisions : 0.0;
        
        // Calculate chi-square for distribution uniformity across shards
        double expected_per_shard = static_cast<double>(num_iterations) / 64.0;
        double chi_square = 0.0;
        for (size_t i = 0; i < 64; ++i) {
            double diff = shard_hash_counts[i] - expected_per_shard;
            chi_square += (diff * diff) / expected_per_shard;
        }
        result.chi_square = chi_square / 63.0; // Chi-square with 63 degrees of freedom, normalized
        
        // Get GoldenHash specific metrics
        if (algo == "goldenhash") {
            result.prime_high = runners[0]->get_result().prime_high;
            result.prime_low = runners[0]->get_result().prime_low;
            result.working_modulus = runners[0]->get_result().working_modulus;
            result.factors = runners[0]->get_result().factors;
        }
        
        // Clean up shards
        for (auto* shard : shards) {
            delete shard;
        }
        
        return result;
    };
    
    if (compare_mode && !json_output) {
        // Test all algorithms
        std::vector<std::string> algorithms = {"goldenhash", "xxhash64", "sha256", "aes-cmac"};
        std::vector<ComparisonResult> results;
        for (const auto& algo : algorithms) {
            results.push_back(run_algorithm_test(algo));
        }
        display_comparison_table(results);
    } else if (!specific_algorithm.empty()) {
        // Test specific algorithm
        auto result = run_algorithm_test(specific_algorithm);
        if (json_output && specific_algorithm == "goldenhash") {
            GoldenHash hasher(table_size);
            output_json_results(result, table_size, num_iterations, hasher);
        }
    } else {
        // Default to goldenhash
        auto result = run_algorithm_test("goldenhash");
        if (json_output) {
            GoldenHash hasher(table_size);
            output_json_results(result, table_size, num_iterations, hasher);
        }
    }
    
    return 0;
}