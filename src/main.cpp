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
#include <filesystem>
#include <algorithm>

using namespace goldenhash;
using namespace goldenhash::tests;

void display_comparison_table(const std::vector<ComparisonResult>& results) {
    if (results.empty()) return;
    
    std::cout << "\n=== HASH ALGORITHM COMPARISON ===\n\n";
    
    // Check if any result has metrics
    bool has_metrics = std::any_of(results.begin(), results.end(), 
                                  [](const auto& r) { return r.metrics_collected; });
    
    // Performance Header
    std::cout << std::left 
              << std::setw(10) << "Algorithm"
              << " | " << std::right
              << std::setw(11) << "Throughput"
              << " | " << std::setw(8) << "ns/hash"
              << " | " << std::setw(8) << "Time(ms)";
    
    if (has_metrics) {
        std::cout << " | " << std::setw(12) << "Avalanche"
                  << " | " << std::setw(12) << "Chi-Squared"
                  << " | " << std::setw(12) << "Coll.Ratio"
                  << " | " << std::setw(10) << "Collisions";
    }
    std::cout << "\n";
    
    std::cout << std::string(10, '-') << "-+-" 
              << std::string(11, '-') << "-+-"
              << std::string(8, '-') << "-+-"
              << std::string(8, '-');
    
    if (has_metrics) {
        std::cout << "-+-" << std::string(12, '-')
                  << "-+-" << std::string(12, '-')
                  << "-+-" << std::string(12, '-')
                  << "-+-" << std::string(10, '-');
    }
    std::cout << "\n";
    
    // Results
    for (const auto& r : results) {
        std::ostringstream throughput_str;
        throughput_str << std::fixed << std::setprecision(1) << r.throughput_mbs << " MB/s";
        
        std::cout << std::left << std::setw(10) << r.algorithm
                  << " | " << std::right
                  << std::setw(11) << throughput_str.str()
                  << " | " << std::setw(8) << std::fixed << std::setprecision(1) << r.ns_per_hash
                  << " | " << std::setw(8) << std::noshowpoint << std::setprecision(0) << r.total_time_ms;
        
        if (has_metrics) {
            if (r.metrics_collected) {
                std::cout << " | " << std::setw(12) << std::fixed << std::setprecision(9) << r.avalanche_score
                          << " | " << std::setw(12) << std::fixed << std::setprecision(9) << r.chi_squared
                          << " | " << std::setw(12) << std::fixed << std::setprecision(9) << r.collision_ratio
                          << " | " << std::setw(10) << r.actual_collisions;
            } else {
                std::cout << " | " << std::setw(12) << "N/A"
                          << " | " << std::setw(12) << "N/A"
                          << " | " << std::setw(12) << "N/A"
                          << " | " << std::setw(10) << "N/A";
            }
        }
        
        std::cout << "\n";
    }
    
    if (has_metrics) {
        std::cout << "\nMetrics Legend:\n";
        std::cout << "  Avalanche: Output bit change rate (0.5 is ideal)\n";
        std::cout << "  Chi-Sq: Distribution uniformity (1.0 is ideal)\n";
        std::cout << "  Coll.Ratio: Actual/Expected collisions (1.0 matches birthday paradox)\n";
        std::cout << "  Collisions: Actual number of hash collisions detected\n";
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
    
    // JSON output
    std::cout << "{\n";
    std::cout << "  \"table_size\": " << table_size << ",\n";
    std::cout << "  \"num_iterations\": " << num_iterations << ",\n";
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
    std::cout << "  \"performance_ns_per_hash\": " << result.ns_per_hash << ",\n";
    std::cout << "  \"throughput_mbs\": " << result.throughput_mbs << "\n";
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
              << "  --metrics          Enable detailed metrics collection (avalanche, chi-squared, collisions)\n"
              << "  --collision-db <path> Store collisions in SQLite database\n"
              << "  --hash-bits <n>    Test with n-bit hashes (default: based on table size)\n"
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
    bool collect_metrics = false;
    std::string specific_algorithm;
    std::string collision_db_path;
    int hash_bits = 0;  // 0 means auto-detect based on table size
    
    // Parse options
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"force-sqlite", no_argument, 0, 's'},
        {"compare", no_argument, 0, 'c'},
        {"algorithm", required_argument, 0, 'a'},
        {"json", no_argument, 0, 'j'},
        {"metrics", no_argument, 0, 'm'},
        {"collision-db", required_argument, 0, 'd'},
        {"hash-bits", required_argument, 0, 'b'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "t:sca:jmd:b:h", long_options, &option_index)) != -1) {
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
            case 'm':
                collect_metrics = true;
                break;
            case 'd':
                collision_db_path = optarg;
                collect_metrics = true;  // Enabling collision db implies metrics
                break;
            case 'b':
                hash_bits = std::stoi(optarg);
                if (hash_bits <= 0 || hash_bits > 64) {
                    std::cerr << "Error: hash-bits must be between 1 and 64\n";
                    return 1;
                }
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
            // Make the data directory if it doesn't exist
            std::filesystem::path p("data/lock.db");
            p = p.parent_path();
            if (!p.empty()) {
                std::error_code ec;
                std::filesystem::create_directories(p, ec);
                if (ec) {
                    throw std::runtime_error("Failed to create directories: " + ec.message());
                }
            }
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

    
    // Lambda to run test for a specific algorithm
    auto run_algorithm_test = [&](const std::string& algo) -> ComparisonResult {
        // Create 64 shards
        std::vector<MapShard*> shards;
        shards.reserve(64);
        
        for (int i = 0; i < 64; ++i) {
            if (use_sqlite) {
                // Include algorithm name in filename to avoid conflicts between algorithms
                // Create data directory if it doesn't exist
                std::filesystem::create_directories("data");
                std::string filename = "data/" + algo + "_shard_" + std::to_string(i) + ".db";
                shards.push_back(new SQLiteShard(filename));
            } else {
                shards.push_back(new HashMapShard());
            }
        }

        std::vector<std::unique_ptr<TestRunner>> runners;
        runners.reserve(num_threads);
        GoldenHash hasher(table_size);
        for (int i = 0; i < num_threads; ++i) {
            runners.emplace_back(std::make_unique<TestRunner>(shards, test_data[i].get(), hasher, algo, table_size));
            
            // Enable metrics collection if requested
            if (collect_metrics && i == 0) {  // Only collect metrics on first thread
                std::string db_path = collision_db_path;
                if (!db_path.empty()) {
                    // Create directory if needed
                    std::filesystem::path p(db_path);
                    if (p.has_parent_path()) {
                        std::filesystem::create_directories(p.parent_path());
                    }
                }
                bool analyze_64bit = (hash_bits == 64);
                runners[i]->enable_metrics(256, db_path, analyze_64bit);
            }
        }

        // Run performance tests first
        if (!json_output) {
            std::cout << "Running performance benchmarks for " << algo << " (" << num_iterations << " iterations across " 
                      << num_threads << " threads)...\n";
        }
        for (auto& runner : runners) {
            runner->run_performance_benchmark();
        }
        
        // Run metrics collection if enabled
        if (collect_metrics) {
            if (!json_output) {
                std::cout << "Collecting quality metrics for " << algo << "...\n";
            }
            runners[0]->run_metrics_collection();
        }

        // Wait for all performance benchmarks to complete
        for (auto& runner : runners) {
            runner->performance_benchmark_complete_.wait(false);
        }
        
        // Wait for metrics collection if enabled
        if (collect_metrics) {
            runners[0]->metrics_collection_complete_.wait(false);
        }
        
        // Aggregate results from all runners
        ComparisonResult result;
        result.algorithm = algo;
        result.table_size = table_size;
        result.ns_per_hash = 0;
        result.throughput_mbs = 0;
        result.total_time_ms = 0;
        result.prime_high = 0;
        result.prime_low = 0;
        result.working_modulus = 0;
        result.factors.clear();
        result.metrics_collected = false;
        
        // Collect performance metrics (average across threads)
        for (const auto& runner : runners) {
            auto runner_result = runner->get_result();
            result.ns_per_hash += runner_result.ns_per_hash;
            result.throughput_mbs += runner_result.throughput_mbs;
            result.total_time_ms = std::max(result.total_time_ms, runner_result.total_time_ms);
        }

        result.ns_per_hash /= runners.size();
        
        // Get GoldenHash specific metrics and quality metrics
        if (algo == "goldenhash") {
            result.prime_high = runners[0]->get_result().prime_high;
            result.prime_low = runners[0]->get_result().prime_low;
            result.working_modulus = runners[0]->get_result().working_modulus;
            result.factors = runners[0]->get_result().factors;
        }
        
        // Get quality metrics if collected
        if (collect_metrics && runners[0]->get_result().metrics_collected) {
            auto metrics_result = runners[0]->get_result();
            result.avalanche_score = metrics_result.avalanche_score;
            result.avalanche_bias = metrics_result.avalanche_bias;
            result.chi_squared = metrics_result.chi_squared;
            result.uniformity_score = metrics_result.uniformity_score;
            result.collision_ratio = metrics_result.collision_ratio;
            result.actual_collisions = metrics_result.actual_collisions;
            result.expected_collisions = metrics_result.expected_collisions;
            result.load_factor = metrics_result.load_factor;
            result.metrics_collected = true;
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
            ComparisonResult algo_result = run_algorithm_test(algo);
            results.push_back(algo_result);
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