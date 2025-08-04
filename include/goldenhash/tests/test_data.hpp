#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <array>
#include <atomic>
#include <thread>
#include <random>

namespace goldenhash::tests {

/**
 * @brief Base class for test data storage
 * 
 * For tests that fit in memory, this can simply be a vector of tests to run through the hash function
 * For larger tests, the tests must be stored in a file or database
 * Destructor must be implemented and clean up after itself
 * Must be thread-safe
 */
class TestData {
public:
    virtual ~TestData() = default;
    virtual void add_test(const std::string& test) = 0;
    virtual void clean_up() = 0;
    virtual std::string get_test(size_t index) = 0;
    virtual size_t size() = 0;
};

/**
 * @brief Creates N number of tests in the test data
 * @param progress_counter Optional atomic counter to track progress
 */
inline static void create_test_data(TestData* data, size_t start_index, size_t end_index, std::atomic<size_t>* progress_counter = nullptr) {
    // Use thread-local random generator to avoid race conditions
    // Use random_device for better randomness instead of predictable seed
    static thread_local std::random_device rd;
    static thread_local std::mt19937 rng(rd());
    const std::array<std::string, 8> test_strings = {
        "",
        "Hello, World!",
        "1234567890",
        "a",
        "abc",
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
        "The quick brown fox jumps over the lazy dog"
    };
    if (start_index >= end_index) {
        throw std::invalid_argument("start_index must be less than end_index");
    }
    size_t number_of_tests = end_index - start_index;
    // Get the total number of tests / 20
    size_t tests_per_iteration = number_of_tests / 20;
    // Adjust the number of tests to be a multiple of 20, we will do the remainder separately
    size_t remainder = number_of_tests % 20;

    size_t progress_batch = 0;
    for (size_t i = 0; i < tests_per_iteration; ++i) {
        size_t current_index = start_index + i * 20;
        // Add test strings (up to 8)
        for (const auto& str : test_strings) {
            if (current_index >= end_index) break;
            if (current_index > 0) {
                data->add_test(str + " " + std::to_string(current_index));
            } else {
                data->add_test(str);
            }
            current_index++;
        }
        
        // Add random alphanumeric strings (up to 8)
        for (size_t j = 0; j < 8 && current_index < end_index; ++j) {
            std::string random_str;
            std::uniform_int_distribution<int> len_dist(8, 31);
            uint8_t len = len_dist(rng); // Random length between 8 and 31
            random_str.resize(len);
            std::uniform_int_distribution<int> char_dist(0, 31);
            for (size_t k = 0; k < len; ++k) {
                random_str[k] = 'a' + char_dist(rng); // Simple lowercase letters and some characters
            }
            if (current_index > 0) {
                data->add_test(random_str + " " + std::to_string(current_index));
            } else {
                data->add_test(random_str);
            }
            current_index++;
        }
        
        // Add random bytes (up to 4)
        std::uniform_int_distribution<int> byte_len_dist(8, 23);
        std::uniform_int_distribution<int> byte_dist(0, 255);
        for (size_t j = 0; j < 4 && current_index < end_index; ++j) {
            std::vector<uint8_t> random_bytes(byte_len_dist(rng)); // Random length between 8 and 23
            for (auto& byte : random_bytes) {
                byte = byte_dist(rng); // Random byte
            }
            std::string byte_str(random_bytes.begin(), random_bytes.end());
            if (current_index > 0) {
                data->add_test(byte_str + " " + std::to_string(current_index));
            } else {
                data->add_test(byte_str);
            }
            current_index++;
            progress_batch++;
            
            // Update progress every 1024 items
            if (progress_counter && progress_batch >= 1024) {
                progress_counter->fetch_add(progress_batch, std::memory_order_relaxed);
                progress_batch = 0;
            }
        }
    }
    // Add the remainder
    for (size_t i = 0; i < remainder; ++i) {
        size_t current_index = start_index + tests_per_iteration * 20 + i;
        if (current_index >= end_index) break;
        // Add a random string
        std::string random_str = "RANDOM_" + std::to_string(current_index);
        data->add_test(random_str);
    }
    // Update any remaining progress
    if (progress_counter && progress_batch > 0) {
        progress_counter->fetch_add(progress_batch, std::memory_order_relaxed);
    }
}

/**
 * @brief In-memory test data implementation
 * 
 * This class stores test data in a vector and provides methods to access it.
 * It is suitable for tests that can fit entirely in memory.
 */
class InMemoryTestData : public TestData {
private:
    std::vector<std::string> tests;
    std::atomic<bool> in_use{false};
public:
    InMemoryTestData(size_t initial_size = 1000) {
        tests.reserve(initial_size);
    }

    void add_test(const std::string& test) override {
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        tests.push_back(test);
        in_use.store(false, std::memory_order_release);
    }

    void clean_up() override {
        tests.clear();
    }

    std::string get_test(size_t index) override {
        if (index >= tests.size()) {
            throw std::out_of_range("Index out of range");
        }
        return tests[index];
    }

    size_t size() override{
        bool expected = false;
        while (!in_use.compare_exchange_strong(expected, true,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
            std::this_thread::yield();
        }
        size_t size = tests.size();
        in_use.store(false, std::memory_order_release);
        return size;
    }
};

}