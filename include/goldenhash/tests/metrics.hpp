/**
 * @file metrics.hpp
 * @brief Hash function quality metrics collection and analysis
 * @author Josh Morgan
 * @date 2025
 * 
 * Provides classes for analyzing hash function quality metrics including
 * avalanche effect, chi-squared distribution, and collision analysis.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <bitset>

namespace goldenhash::tests {

/**
 * @brief Analyzes avalanche effect - how output bits change when input bits change
 * 
 * The avalanche effect measures how many output bits change when a single
 * input bit is flipped. Ideal value is 0.5 (50% of bits change).
 */
class AvalancheAnalyzer {
private:
    size_t output_bits_;
    size_t total_tests_ = 0;
    size_t total_bit_changes_ = 0;
    std::vector<double> bit_change_probabilities_;

public:
    /**
     * @brief Constructor
     * @param output_bits Number of bits in hash output to analyze
     */
    explicit AvalancheAnalyzer(size_t output_bits = 64) 
        : output_bits_(output_bits), bit_change_probabilities_(output_bits, 0.0) {}

    /**
     * @brief Analyze avalanche effect between two hash values
     * @param hash1 First hash value
     * @param hash2 Second hash value (should differ by 1 bit in input)
     */
    void add_sample(uint64_t hash1, uint64_t hash2) {
        uint64_t diff = hash1 ^ hash2;
        size_t bits_changed = __builtin_popcountll(diff);
        total_bit_changes_ += bits_changed;
        total_tests_++;

        // Track which bits changed
        for (size_t i = 0; i < output_bits_; ++i) {
            if (diff & (1ULL << i)) {
                bit_change_probabilities_[i]++;
            }
        }
    }

    /**
     * @brief Get the average avalanche score
     * @return Score between 0 and 1 (0.5 is ideal)
     */
    double get_avalanche_score() const {
        if (total_tests_ == 0) return 0.0;
        return static_cast<double>(total_bit_changes_) / (total_tests_ * output_bits_);
    }

    /**
     * @brief Get per-bit change probabilities
     * @return Vector of probabilities for each bit position
     */
    std::vector<double> get_bit_probabilities() const {
        if (total_tests_ == 0) return bit_change_probabilities_;
        
        std::vector<double> probs = bit_change_probabilities_;
        for (auto& p : probs) {
            p /= total_tests_;
        }
        return probs;
    }

    /**
     * @brief Get avalanche bias (deviation from ideal 0.5)
     * @return Bias value (0 is perfect, higher is worse)
     */
    double get_avalanche_bias() const {
        auto probs = get_bit_probabilities();
        double sum_squared_deviation = 0.0;
        
        for (double p : probs) {
            double deviation = p - 0.5;
            sum_squared_deviation += deviation * deviation;
        }
        
        return std::sqrt(sum_squared_deviation / output_bits_);
    }
};

/**
 * @brief Calculates chi-squared statistic for hash distribution uniformity
 * 
 * Tests whether hash values are uniformly distributed across buckets.
 * Lower chi-squared values indicate better distribution.
 */
class ChiSquaredCalculator {
private:
    size_t num_buckets_;
    std::vector<size_t> bucket_counts_;
    size_t total_samples_ = 0;

public:
    /**
     * @brief Constructor
     * @param num_buckets Number of buckets for distribution analysis
     */
    explicit ChiSquaredCalculator(size_t num_buckets) 
        : num_buckets_(num_buckets), bucket_counts_(num_buckets, 0) {}

    /**
     * @brief Add a hash value sample
     * @param hash_value Hash value to add
     * @param table_size Size of hash table (for modulo operation)
     */
    void add_sample(uint64_t hash_value, uint64_t table_size) {
        size_t bucket = (hash_value % table_size) % num_buckets_;
        bucket_counts_[bucket]++;
        total_samples_++;
    }

    /**
     * @brief Calculate chi-squared statistic
     * @return Chi-squared value (lower is better, 1.0 is ideal for large samples)
     */
    double get_chi_squared() const {
        if (total_samples_ == 0) return 0.0;
        
        double expected = static_cast<double>(total_samples_) / num_buckets_;
        double chi_squared = 0.0;
        
        for (size_t count : bucket_counts_) {
            double deviation = count - expected;
            chi_squared += (deviation * deviation) / expected;
        }
        
        // Normalize by degrees of freedom
        return chi_squared / (num_buckets_ - 1);
    }

    /**
     * @brief Get the distribution uniformity score
     * @return Score between 0 and 1 (1 is perfectly uniform)
     */
    double get_uniformity_score() const {
        if (total_samples_ == 0) return 0.0;
        
        // Calculate entropy
        double entropy = 0.0;
        double max_entropy = std::log2(num_buckets_);
        
        for (size_t count : bucket_counts_) {
            if (count > 0) {
                double probability = static_cast<double>(count) / total_samples_;
                entropy -= probability * std::log2(probability);
            }
        }
        
        return entropy / max_entropy;
    }

    /**
     * @brief Get bucket statistics
     * @return Pair of (min_count, max_count)
     */
    std::pair<size_t, size_t> get_bucket_stats() const {
        if (bucket_counts_.empty()) return {0, 0};
        
        auto [min_it, max_it] = std::minmax_element(bucket_counts_.begin(), bucket_counts_.end());
        return {*min_it, *max_it};
    }
};

/**
 * @brief Analyzes hash collisions and compares to birthday paradox predictions
 * 
 * Tracks actual collisions and compares to theoretical expectations based
 * on the birthday paradox for hash functions.
 */
class CollisionAnalyzer {
private:
    std::unordered_map<uint64_t, size_t> hash_counts_;
    size_t total_hashes_ = 0;
    size_t actual_collisions_ = 0;
    uint64_t hash_space_size_;
    std::vector<std::pair<uint64_t, std::vector<size_t>>> collision_details_;

public:
    /**
     * @brief Constructor
     * @param hash_space_size Size of the hash space (e.g., table_size for modulo hashes)
     */
    explicit CollisionAnalyzer(uint64_t hash_space_size) 
        : hash_space_size_(hash_space_size) {}

    /**
     * @brief Add a hash value and check for collisions
     * @param hash_value Hash value to add
     * @param input_index Index of the input that produced this hash
     */
    void add_hash(uint64_t hash_value, size_t input_index = 0) {
        auto& count = hash_counts_[hash_value];
        if (count > 0) {
            actual_collisions_++;
            
            // Track collision details for first few collisions
            if (collision_details_.size() < 100) {
                auto it = std::find_if(collision_details_.begin(), collision_details_.end(),
                    [hash_value](const auto& p) { return p.first == hash_value; });
                
                if (it != collision_details_.end()) {
                    it->second.push_back(input_index);
                } else {
                    collision_details_.push_back({hash_value, {input_index}});
                }
            }
        }
        count++;
        total_hashes_++;
    }

    /**
     * @brief Get expected collisions based on birthday paradox
     * @return Expected number of collisions
     */
    double get_expected_collisions() const {
        if (total_hashes_ <= 1) return 0.0;
        
        // Birthday paradox approximation: n^2 / (2m)
        // where n = number of hashes, m = hash space size
        double n = static_cast<double>(total_hashes_);
        double m = static_cast<double>(hash_space_size_);
        
        // More accurate calculation for smaller values
        if (total_hashes_ < 1000) {
            // Using the exact formula: expected collisions = n - m(1 - (1-1/m)^n)
            // But we already have the unique count, so:
            return n - hash_counts_.size();
        }
        
        // Approximation for larger values
        return (n * n) / (2.0 * m);
    }

    /**
     * @brief Get collision ratio (actual / expected)
     * @return Ratio where 1.0 means matching birthday paradox prediction
     */
    double get_collision_ratio() const {
        double expected = get_expected_collisions();
        if (expected == 0) return 0.0;
        return actual_collisions_ / expected;
    }

    /**
     * @brief Get number of unique hash values
     * @return Count of unique hashes
     */
    size_t get_unique_hashes() const {
        return hash_counts_.size();
    }

    /**
     * @brief Get actual collision count
     * @return Number of collisions detected
     */
    size_t get_actual_collisions() const {
        return actual_collisions_;
    }

    /**
     * @brief Get collision details for analysis
     * @return Vector of collision hash values and their input indices
     */
    const std::vector<std::pair<uint64_t, std::vector<size_t>>>& get_collision_details() const {
        return collision_details_;
    }

    /**
     * @brief Get load factor (unique hashes / hash space size)
     * @return Load factor between 0 and 1
     */
    double get_load_factor() const {
        return static_cast<double>(hash_counts_.size()) / hash_space_size_;
    }
};

/**
 * @brief Aggregates all metrics for a hash algorithm
 */
struct HashMetrics {
    // Avalanche metrics
    double avalanche_score = 0.0;
    double avalanche_bias = 0.0;
    
    // Distribution metrics
    double chi_squared = 0.0;
    double uniformity_score = 0.0;
    
    // Collision metrics
    double collision_ratio = 0.0;
    size_t actual_collisions = 0;
    double expected_collisions = 0.0;
    double load_factor = 0.0;
    
    // Performance metrics (existing)
    double throughput_mbs = 0.0;
    double ns_per_hash = 0.0;
};

} // namespace goldenhash::tests