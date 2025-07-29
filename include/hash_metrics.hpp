/**
 * @file hash_metrics.hpp
 * @brief Hash function metrics and testing framework
 * @author Josh Morgan
 * @date 2025
 * 
 * This file implements a comprehensive testing framework for hash functions,
 * including standard tests like avalanche effect, collision analysis, 
 * distribution uniformity, and performance benchmarks.
 */

#ifndef HASH_METRICS_HPP
#define HASH_METRICS_HPP

#include "inmemory_hash.hpp"
#include <chrono>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>
#include <bitset>

#ifdef __has_include
#  if __has_include(<json/json.h>)
#    include <json/json.h>
#    define HAS_JSONCPP 1
#  else
#    define HAS_JSONCPP 0
#  endif
#else
#  define HAS_JSONCPP 0
#endif

namespace goldenhash {

/**
 * @struct MetricResult
 * @brief Container for individual metric results
 */
struct MetricResult {
    std::string name;
    double value;
    std::string unit;
    std::string description;
    std::map<std::string, double> details;
};

/**
 * @struct TestConfiguration
 * @brief Configuration for hash function tests
 */
struct TestConfiguration {
    uint64_t num_keys = 1000000;
    uint64_t table_size = 1024;
    uint64_t seed = 0;
    bool test_avalanche = true;
    bool test_distribution = true;
    bool test_collisions = true;
    bool test_performance = true;
    bool test_bit_independence = true;
    uint32_t num_performance_runs = 10;
    uint32_t avalanche_samples = 10000;
};

/**
 * @class HashMetrics
 * @brief Comprehensive hash function testing and metrics
 */
class HashMetrics {
private:
    std::mt19937_64 rng_;
    
    /**
     * @brief Count the number of differing bits between two values
     */
    static uint32_t count_bit_differences(uint64_t a, uint64_t b) {
        return std::bitset<64>(a ^ b).count();
    }
    
    /**
     * @brief Calculate chi-squared statistic for distribution test
     */
    double calculate_chi_squared(const std::vector<uint64_t>& buckets, double expected) const {
        double chi_squared = 0.0;
        for (const auto& count : buckets) {
            double diff = static_cast<double>(count) - expected;
            chi_squared += (diff * diff) / expected;
        }
        return chi_squared;
    }
    
public:
    /**
     * @brief Constructor
     * @param seed Random seed for test data generation
     */
    explicit HashMetrics(uint64_t seed = std::random_device{}()) : rng_(seed) {}
    
    /**
     * @brief Run all configured tests on a hash function
     * @param hash_func The hash function to test
     * @param config Test configuration
     * @return Vector of metric results
     */
    std::vector<MetricResult> run_tests(const InMemoryHash& hash_func, 
                                       const TestConfiguration& config) {
        std::vector<MetricResult> results;
        
        // Generate test keys
        std::vector<uint64_t> test_keys = generate_test_keys(config.num_keys);
        
        if (config.test_avalanche) {
            auto avalanche_result = test_avalanche_effect(hash_func, config);
            results.push_back(avalanche_result);
        }
        
        if (config.test_distribution) {
            auto dist_result = test_distribution_uniformity(hash_func, test_keys, config);
            results.push_back(dist_result);
        }
        
        if (config.test_collisions) {
            auto collision_result = test_collision_rate(hash_func, test_keys, config);
            results.push_back(collision_result);
        }
        
        if (config.test_performance) {
            auto perf_result = test_performance(hash_func, test_keys, config);
            results.push_back(perf_result);
        }
        
        if (config.test_bit_independence) {
            auto bit_result = test_bit_independence(hash_func, config);
            results.push_back(bit_result);
        }
        
        return results;
    }
    
    /**
     * @brief Test avalanche effect (changing 1 input bit affects ~50% output bits)
     */
    MetricResult test_avalanche_effect(const InMemoryHash& hash_func, 
                                      const TestConfiguration& config) {
        MetricResult result;
        result.name = "avalanche_effect";
        result.description = "Average bit flip ratio when input bit changes";
        result.unit = "ratio";
        
        std::vector<double> bit_flip_ratios;
        std::uniform_int_distribution<uint64_t> key_dist;
        
        for (uint32_t i = 0; i < config.avalanche_samples; ++i) {
            uint64_t original_key = key_dist(rng_);
            uint64_t original_hash = hash_func.supports_seed() 
                ? hash_func.hash_with_seed(original_key, config.seed, config.table_size)
                : hash_func.hash(original_key, config.table_size);
            
            // Test each bit position
            for (int bit = 0; bit < 64; ++bit) {
                uint64_t flipped_key = original_key ^ (1ULL << bit);
                uint64_t flipped_hash = hash_func.supports_seed()
                    ? hash_func.hash_with_seed(flipped_key, config.seed, config.table_size)
                    : hash_func.hash(flipped_key, config.table_size);
                
                uint32_t bits_changed = count_bit_differences(original_hash, flipped_hash);
                bit_flip_ratios.push_back(bits_changed / 64.0);
            }
        }
        
        // Calculate statistics
        double mean = std::accumulate(bit_flip_ratios.begin(), bit_flip_ratios.end(), 0.0) 
                     / bit_flip_ratios.size();
        
        double variance = 0.0;
        for (const auto& ratio : bit_flip_ratios) {
            variance += (ratio - mean) * (ratio - mean);
        }
        variance /= bit_flip_ratios.size();
        
        result.value = mean;
        result.details["mean"] = mean;
        result.details["variance"] = variance;
        result.details["std_dev"] = std::sqrt(variance);
        result.details["ideal"] = 0.5;
        result.details["deviation_from_ideal"] = std::abs(mean - 0.5);
        
        return result;
    }
    
    /**
     * @brief Test distribution uniformity using chi-squared test
     */
    MetricResult test_distribution_uniformity(const InMemoryHash& hash_func,
                                            const std::vector<uint64_t>& keys,
                                            const TestConfiguration& config) {
        MetricResult result;
        result.name = "distribution_uniformity";
        result.description = "Chi-squared test for uniform distribution";
        result.unit = "chi_squared";
        
        // Create buckets
        std::vector<uint64_t> buckets(config.table_size, 0);
        
        // Hash all keys and count bucket usage
        for (const auto& key : keys) {
            uint64_t hash_val = hash_func.supports_seed()
                ? hash_func.hash_with_seed(key, config.seed, config.table_size)
                : hash_func.hash(key, config.table_size);
            
            buckets[hash_val % config.table_size]++;
        }
        
        // Calculate expected count per bucket
        double expected = static_cast<double>(keys.size()) / config.table_size;
        
        // Calculate chi-squared statistic
        double chi_squared = calculate_chi_squared(buckets, expected);
        
        // Find min/max bucket counts
        auto [min_it, max_it] = std::minmax_element(buckets.begin(), buckets.end());
        
        result.value = chi_squared;
        result.details["chi_squared"] = chi_squared;
        result.details["expected_per_bucket"] = expected;
        result.details["min_bucket_count"] = static_cast<double>(*min_it);
        result.details["max_bucket_count"] = static_cast<double>(*max_it);
        result.details["degrees_of_freedom"] = static_cast<double>(config.table_size - 1);
        
        return result;
    }
    
    /**
     * @brief Test collision rate
     */
    MetricResult test_collision_rate(const InMemoryHash& hash_func,
                                   const std::vector<uint64_t>& keys,
                                   const TestConfiguration& config) {
        MetricResult result;
        result.name = "collision_rate";
        result.description = "Ratio of keys that collide";
        result.unit = "ratio";
        
        std::map<uint64_t, uint64_t> hash_counts;
        uint64_t total_collisions = 0;
        
        for (const auto& key : keys) {
            uint64_t hash_val = hash_func.supports_seed()
                ? hash_func.hash_with_seed(key, config.seed, config.table_size)
                : hash_func.hash(key, config.table_size);
            
            hash_val = hash_val % config.table_size;
            
            if (hash_counts[hash_val]++ > 0) {
                total_collisions++;
            }
        }
        
        double collision_rate = static_cast<double>(total_collisions) / keys.size();
        
        // Calculate expected collisions (birthday paradox)
        double n = static_cast<double>(keys.size());
        double m = static_cast<double>(config.table_size);
        double expected_collisions = n - m * (1.0 - std::pow(1.0 - 1.0/m, n));
        double expected_rate = expected_collisions / n;
        
        result.value = collision_rate;
        result.details["collision_rate"] = collision_rate;
        result.details["total_collisions"] = static_cast<double>(total_collisions);
        result.details["unique_hashes"] = static_cast<double>(hash_counts.size());
        result.details["expected_rate"] = expected_rate;
        result.details["deviation_from_expected"] = std::abs(collision_rate - expected_rate);
        
        return result;
    }
    
    /**
     * @brief Test hash function performance
     */
    MetricResult test_performance(const InMemoryHash& hash_func,
                                const std::vector<uint64_t>& keys,
                                const TestConfiguration& config) {
        MetricResult result;
        result.name = "performance";
        result.description = "Average time per hash operation";
        result.unit = "nanoseconds";
        
        std::vector<double> timings;
        
        for (uint32_t run = 0; run < config.num_performance_runs; ++run) {
            auto start = std::chrono::high_resolution_clock::now();
            
            if (hash_func.supports_seed()) {
                hash_func.hash_batch_with_seed(keys, config.seed, config.table_size);
            } else {
                hash_func.hash_batch(keys, config.table_size);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            double ns_per_hash = static_cast<double>(duration.count()) / keys.size();
            timings.push_back(ns_per_hash);
        }
        
        // Calculate statistics
        std::sort(timings.begin(), timings.end());
        double median = timings[timings.size() / 2];
        double mean = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();
        
        result.value = median;
        result.details["median_ns"] = median;
        result.details["mean_ns"] = mean;
        result.details["min_ns"] = timings.front();
        result.details["max_ns"] = timings.back();
        result.details["hashes_per_second"] = 1e9 / median;
        
        return result;
    }
    
    /**
     * @brief Test bit independence
     */
    MetricResult test_bit_independence(const InMemoryHash& hash_func,
                                     const TestConfiguration& config) {
        MetricResult result;
        result.name = "bit_independence";
        result.description = "Correlation between output bit positions";
        result.unit = "max_correlation";
        
        const uint32_t num_samples = 10000;
        std::vector<std::bitset<64>> hash_bits;
        std::uniform_int_distribution<uint64_t> key_dist;
        
        // Collect hash values
        for (uint32_t i = 0; i < num_samples; ++i) {
            uint64_t key = key_dist(rng_);
            uint64_t hash_val = hash_func.supports_seed()
                ? hash_func.hash_with_seed(key, config.seed, config.table_size)
                : hash_func.hash(key, config.table_size);
            hash_bits.push_back(std::bitset<64>(hash_val));
        }
        
        // Calculate bit correlations
        double max_correlation = 0.0;
        
        for (int bit1 = 0; bit1 < 64; ++bit1) {
            for (int bit2 = bit1 + 1; bit2 < 64; ++bit2) {
                uint32_t both_set = 0;
                uint32_t bit1_set = 0;
                uint32_t bit2_set = 0;
                
                for (const auto& bits : hash_bits) {
                    if (bits[bit1]) bit1_set++;
                    if (bits[bit2]) bit2_set++;
                    if (bits[bit1] && bits[bit2]) both_set++;
                }
                
                double expected = (static_cast<double>(bit1_set) * bit2_set) / num_samples;
                double correlation = std::abs(both_set - expected) / num_samples;
                max_correlation = std::max(max_correlation, correlation);
            }
        }
        
        result.value = max_correlation;
        result.details["max_correlation"] = max_correlation;
        result.details["ideal_correlation"] = 0.0;
        
        return result;
    }
    
    /**
     * @brief Export results to JSON string
     */
    std::string export_to_json(const std::vector<MetricResult>& results,
                              const InMemoryHash& hash_func,
                              const TestConfiguration& config) const {
        std::stringstream json;
        json << "{\n";
        json << "  \"hash_function\": {\n";
        json << "    \"name\": \"" << hash_func.get_name() << "\",\n";
        json << "    \"description\": \"" << hash_func.get_description() << "\",\n";
        json << "    \"supports_seed\": " << (hash_func.supports_seed() ? "true" : "false") << ",\n";
        json << "    \"uses_floating_point\": " << (hash_func.uses_floating_point() ? "true" : "false") << "\n";
        json << "  },\n";
        json << "  \"configuration\": {\n";
        json << "    \"num_keys\": " << config.num_keys << ",\n";
        json << "    \"table_size\": " << config.table_size << ",\n";
        json << "    \"seed\": " << config.seed << "\n";
        json << "  },\n";
        json << "  \"results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            json << "    {\n";
            json << "      \"name\": \"" << result.name << "\",\n";
            json << "      \"value\": " << std::setprecision(10) << result.value << ",\n";
            json << "      \"unit\": \"" << result.unit << "\",\n";
            json << "      \"description\": \"" << result.description << "\",\n";
            json << "      \"details\": {\n";
            
            size_t detail_idx = 0;
            for (const auto& [key, value] : result.details) {
                json << "        \"" << key << "\": " << std::setprecision(10) << value;
                if (++detail_idx < result.details.size()) json << ",";
                json << "\n";
            }
            
            json << "      }\n";
            json << "    }";
            if (i < results.size() - 1) json << ",";
            json << "\n";
        }
        
        json << "  ]\n";
        json << "}\n";
        
        return json.str();
    }
    
private:
    /**
     * @brief Generate test keys
     */
    std::vector<uint64_t> generate_test_keys(uint64_t count) {
        std::vector<uint64_t> keys;
        keys.reserve(count);
        
        std::uniform_int_distribution<uint64_t> dist;
        for (uint64_t i = 0; i < count; ++i) {
            keys.push_back(dist(rng_));
        }
        
        return keys;
    }
};

} // namespace goldenhash

#endif // HASH_METRICS_HPP