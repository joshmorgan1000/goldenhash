#include <goldenhash.hpp>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

int main() {
    const uint64_t table_size = 1048576;
    const size_t num_tests = 1000000;
    
    goldenhash::GoldenHash hasher(table_size);
    
    // Generate test data
    std::mt19937_64 rng(42);
    std::vector<std::vector<uint8_t>> test_data;
    size_t total_bytes = 0;
    
    for (size_t i = 0; i < num_tests; i++) {
        std::vector<uint8_t> data(16 + (i % 48)); // Vary size 16-64 bytes
        for (auto& byte : data) {
            byte = rng() & 0xFF;
        }
        total_bytes += data.size();
        test_data.push_back(std::move(data));
    }
    
    // Warm up
    for (size_t i = 0; i < 1000; i++) {
        hasher.hash(test_data[i].data(), test_data[i].size());
    }
    
    // Single-threaded benchmark
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& data : test_data) {
        volatile uint64_t h = hasher.hash(data.data(), data.size());
        (void)h; // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double ns_per_hash = static_cast<double>(duration.count()) / num_tests;
    double throughput_mbs = (total_bytes / (1024.0 * 1024.0)) / (duration.count() / 1e9);
    
    std::cout << "Single-threaded performance:\n";
    std::cout << "Throughput: " << throughput_mbs << " MB/s\n";
    std::cout << "ns/hash: " << ns_per_hash << "\n";
    
    return 0;
}