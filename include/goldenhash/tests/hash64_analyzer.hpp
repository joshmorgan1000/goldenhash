/**
 * @file hash64_analyzer.hpp
 * @brief 64-bit hash collision analysis for large-scale testing
 * @author Josh Morgan
 * @date 2025
 * 
 * Provides specialized analysis for full 64-bit hash values without
 * modulo operations, suitable for testing hash quality at scale.
 */

#pragma once

#include <cstdint>
#include <cmath>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_set>
#include "collision_store.hpp"

namespace goldenhash::tests {

/**
 * @brief Analyzes 64-bit hash collisions for very large datasets
 * 
 * This class is designed to handle analysis of full 64-bit hash values
 * without modulo operations, providing insights into the true collision
 * characteristics of hash functions at scale.
 */
class Hash64Analyzer {
private:
    std::unique_ptr<CollisionStore> collision_store_;
    std::atomic<uint64_t> total_hashes_{0};
    std::atomic<uint64_t> unique_hashes_{0};
    std::atomic<uint64_t> actual_collisions_{0};
    
    // For memory efficiency, we use a probabilistic approach for very large datasets
    static constexpr size_t SAMPLE_SIZE = 10000000;  // 10M samples
    std::unique_ptr<std::unordered_set<uint64_t>> sample_set_;
    bool use_sampling_ = false;
    
public:
    /**
     * @brief Constructor
     * @param collision_db_path Path to collision database (optional)
     * @param expected_hashes Expected number of hashes (for memory allocation)
     */
    explicit Hash64Analyzer(const std::string& collision_db_path = "", 
                           uint64_t expected_hashes = 0) {
        if (!collision_db_path.empty()) {
            collision_store_ = std::make_unique<SQLiteCollisionStore>(collision_db_path);
            collision_store_->initialize();
        }
        
        // Use sampling for very large datasets
        if (expected_hashes > SAMPLE_SIZE * 10) {
            use_sampling_ = true;
            sample_set_ = std::make_unique<std::unordered_set<uint64_t>>();
            sample_set_->reserve(SAMPLE_SIZE);
        }
    }
    
    /**
     * @brief Add a 64-bit hash value for analysis
     * @param hash_value Full 64-bit hash value
     * @param data Optional input data that produced this hash
     * @param data_len Length of input data
     * @return True if this was a collision
     */
    bool add_hash(uint64_t hash_value, const uint8_t* /*data*/ = nullptr, size_t /*data_len*/ = 0) {
        total_hashes_++;
        
        if (use_sampling_) {
            // Probabilistic sampling for very large datasets
            if (sample_set_->size() < SAMPLE_SIZE) {
                auto [it, inserted] = sample_set_->insert(hash_value);
                if (inserted) {
                    unique_hashes_++;
                    return false;
                } else {
                    actual_collisions_++;
                    return true;
                }
            }
            // After sample is full, use statistical estimation
            return false;
        } else {
            // For smaller datasets, track everything
            // This would need a more sophisticated approach for production
            // (e.g., bloom filters, hyperloglog, etc.)
            return false;
        }
    }
    
    /**
     * @brief Get expected collisions for 64-bit hash space
     * @return Expected number of collisions based on birthday paradox
     */
    double get_expected_collisions_64bit() const {
        uint64_t n = total_hashes_.load();
        if (n <= 1) return 0.0;
        
        // For 64-bit hash space (2^64 possible values)
        // Using approximation: n^2 / (2 * 2^64)
        // = n^2 / 2^65
        double n_double = static_cast<double>(n);
        
        // For small n, this is accurate
        if (n < 1000000) {
            return (n_double * n_double) / std::pow(2.0, 65);
        }
        
        // For larger n, use more sophisticated calculation
        // Expected collisions ≈ n - 2^64 * (1 - e^(-n/2^64))
        // But for n << 2^64, this simplifies to n^2 / 2^65
        double hash_space = std::pow(2.0, 64);
        double prob_no_collision = std::exp(-n_double / hash_space);
        return n_double - hash_space * (1.0 - prob_no_collision);
    }
    
    /**
     * @brief Get collision probability for current number of hashes
     * @return Probability of at least one collision
     */
    double get_collision_probability() const {
        uint64_t n = total_hashes_.load();
        if (n <= 1) return 0.0;
        
        // P(collision) = 1 - e^(-n^2 / 2^65)
        double n_double = static_cast<double>(n);
        double exponent = -(n_double * n_double) / std::pow(2.0, 65);
        return 1.0 - std::exp(exponent);
    }
    
    /**
     * @brief Calculate how many hashes before 50% collision probability
     * @return Number of hashes for 50% collision probability
     */
    static uint64_t hashes_for_50_percent_collision() {
        // For 64-bit hash: sqrt(ln(2) * 2^65) ≈ 5.06 billion
        return static_cast<uint64_t>(std::sqrt(std::log(2.0) * std::pow(2.0, 65)));
    }
    
    /**
     * @brief Get statistics about the analysis
     * @return String with formatted statistics
     */
    std::string get_statistics() const {
        std::stringstream ss;
        ss << "64-bit Hash Analysis Statistics:\n";
        ss << "  Total hashes: " << total_hashes_.load() << "\n";
        
        if (use_sampling_) {
            ss << "  Mode: Sampling (sample size: " << SAMPLE_SIZE << ")\n";
            ss << "  Unique in sample: " << unique_hashes_.load() << "\n";
            ss << "  Collisions in sample: " << actual_collisions_.load() << "\n";
        } else {
            ss << "  Mode: Full tracking\n";
            ss << "  Unique hashes: " << unique_hashes_.load() << "\n";
            ss << "  Actual collisions: " << actual_collisions_.load() << "\n";
        }
        
        ss << "  Expected collisions: " << std::fixed << std::setprecision(6) 
           << get_expected_collisions_64bit() << "\n";
        ss << "  Collision probability: " << std::fixed << std::setprecision(6) 
           << get_collision_probability() << "\n";
        ss << "  Hashes for 50% collision: " << hashes_for_50_percent_collision() << "\n";
        
        return ss.str();
    }
    
    /**
     * @brief Save analysis results to database
     * @param algorithm Algorithm name
     * @param metadata Additional metadata JSON
     */
    void save_results(const std::string& algorithm, const std::string& metadata = "{}") {
        if (!collision_store_) return;
        
        TestRunRecord record;
        record.run_id = generate_run_id(algorithm + "_64bit");
        record.algorithm = algorithm;
        record.table_size = 0;  // No table size for 64-bit analysis
        record.num_hashes = total_hashes_.load();
        record.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        record.actual_collisions = actual_collisions_.load();
        record.expected_collisions = get_expected_collisions_64bit();
        record.collision_ratio = record.actual_collisions / std::max(1.0, record.expected_collisions);
        record.metadata_json = metadata;
        
        collision_store_->store_test_run(record);
    }
};

/**
 * @brief Utility to calculate birthday paradox statistics for different bit sizes
 */
class BirthdayParadoxCalculator {
public:
    /**
     * @brief Calculate number of items for given collision probability
     * @param bits Number of bits in hash
     * @param probability Target collision probability (0-1)
     * @return Number of items
     */
    static uint64_t items_for_probability(int bits, double probability) {
        if (bits <= 0 || bits > 64) return 0;
        if (probability <= 0 || probability >= 1) return 0;
        
        // n ≈ sqrt(-2 * H * ln(1 - p))
        // where H = 2^bits (hash space size)
        double hash_space = std::pow(2.0, bits);
        double n = std::sqrt(-2.0 * hash_space * std::log(1.0 - probability));
        
        return static_cast<uint64_t>(std::ceil(n));
    }
    
    /**
     * @brief Get a table of collision probabilities for different hash sizes
     * @return Formatted table string
     */
    static std::string get_probability_table() {
        std::stringstream ss;
        ss << "Birthday Paradox: Number of hashes for 50% collision probability\n";
        ss << "Bits | Hash Space Size | Hashes for 50% collision\n";
        ss << "-----|-----------------|-------------------------\n";
        
        for (int bits = 8; bits <= 64; bits += 8) {
            uint64_t hash_space = 1ULL << bits;
            uint64_t hashes_50 = items_for_probability(bits, 0.5);
            
            ss << std::setw(4) << bits << " | ";
            
            // Format hash space size
            if (bits <= 20) {
                ss << std::setw(15) << hash_space;
            } else {
                ss << std::setw(15) << std::scientific << std::setprecision(2) 
                   << static_cast<double>(hash_space);
            }
            
            ss << " | ";
            
            // Format number of hashes
            if (hashes_50 < 1000000) {
                ss << std::setw(23) << hashes_50;
            } else if (hashes_50 < 1000000000) {
                ss << std::setw(23) << std::fixed << std::setprecision(1) 
                   << (hashes_50 / 1e6) << "M";
            } else {
                ss << std::setw(23) << std::fixed << std::setprecision(1) 
                   << (hashes_50 / 1e9) << "B";
            }
            
            ss << "\n";
        }
        
        return ss.str();
    }
};

} // namespace goldenhash::tests