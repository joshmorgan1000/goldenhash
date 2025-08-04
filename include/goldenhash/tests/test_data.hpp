#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <array>
#include <atomic>
#include <thread>

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
 */
inline static void create_test_data(TestData* data, size_t start_index, size_t end_index) {
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

    for (size_t i = start_index; i < end_index; i += 20) {
        for (const auto& str : test_strings) {
            if (i > 0) {
                data->add_test(str + " " + std::to_string(i));
            } else {
                data->add_test(str);
            }
        }
        // For the next 8, add some random alphanumeric strings
        for (size_t j = 0; j < 8; ++j) {
            std::string random_str;
            uint8_t len = 8 + (rand() % 24); // Random length between 8 and 32
            random_str.resize(len);
            for (size_t k = 0; k < len; ++k) {
                random_str[k] = 'a' + (rand() & 0b00011111); // Simple lowercase letters and some numbers
            }
            if (i > 0) {
                data->add_test(random_str + " " + std::to_string(i));
            } else {
                data->add_test(random_str);
            }
        }
        // For the next 4, add random bytes to the test data
        for (size_t j = 0; j < 4; ++j) {
            std::vector<uint8_t> random_bytes(8 + (rand() & 0b00001111)); // Random length between 8 and 24
            for (auto& byte : random_bytes) {
                byte = rand() & 0xFF; // Random byte
            }
            std::string byte_str(random_bytes.begin(), random_bytes.end());
            if (i > 0) {
                data->add_test(byte_str + " " + std::to_string(i));
            } else {
                data->add_test(byte_str);
            }
        }
    }
    // For the remaining modulo 20 tests, add some specific strings
    if (remainder == 0) return; // No remainder, nothing to add
    for (size_t i = (end_index - remainder); i < end_index; ++i) {
        if (i < test_strings.size()) {
            data->add_test(test_strings[i]);
        } else {
            std::string random_str;
            uint8_t len = 8 + (rand() & 0b00001111); // Random length between 8 and 24
            random_str.resize(len);
            for (size_t j = 0; j < len; ++j) {
                random_str[j] = 'a' + (rand() & 0b00011111); // Simple lowercase letters and some numbers
            }
            data->add_test(random_str);
        }
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