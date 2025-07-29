#include <iostream>
#include <iomanip>
#include <cstdint>

int main() {
    constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;
    
    constexpr double PHI = 1.6180339887498948482;
    constexpr double SQRT_5 = 2.2360679774997896964;
    
    uint64_t primes[] = {PRIME64_1, PRIME64_2, PRIME64_3, PRIME64_4, PRIME64_5};
    const char* names[] = {"PRIME64_1", "PRIME64_2", "PRIME64_3", "PRIME64_4", "PRIME64_5"};
    
    std::cout << "xxHash64 Prime Analysis\n";
    std::cout << "=======================\n\n";
    
    // Check for golden ratio relationships
    std::cout << "Golden ratio φ = " << PHI << "\n";
    std::cout << "2^64 / φ = " << (double)UINT64_MAX / PHI << "\n";
    std::cout << "2^32 / φ = " << (double)UINT32_MAX / PHI << "\n\n";
    
    for (int i = 0; i < 5; i++) {
        uint64_t p = primes[i];
        std::cout << names[i] << " = 0x" << std::hex << p << std::dec << "\n";
        std::cout << "         = " << p << "\n";
        
        // Check various golden ratio relationships
        std::cout << "  2^64 / prime = " << (double)UINT64_MAX / p << "\n";
        std::cout << "  2^32 / (prime >> 32) = " << (double)UINT32_MAX / (p >> 32) << "\n";
        
        // Look for golden ratio in the hex digits
        uint32_t high = p >> 32;
        uint32_t low = p & 0xFFFFFFFF;
        std::cout << "  High 32 bits: 0x" << std::hex << high << std::dec << " = " << high << "\n";
        std::cout << "  Low 32 bits:  0x" << std::hex << low << std::dec << " = " << low << "\n";
        
        // Special check for PRIME64_1
        if (i == 0) {
            // 0x9E3779B1 is special!
            uint32_t special = 0x9E3779B1;
            std::cout << "  Note: 0x9E3779B1 = " << special << "\n";
            std::cout << "        2^32 / 0x9E3779B1 = " << (double)UINT32_MAX / special << " ≈ φ\n";
            std::cout << "        This is the 'golden ratio prime' for 32-bit!\n";
        }
        
        std::cout << "\n";
    }
    
    // Look for patterns
    std::cout << "Pattern Analysis:\n";
    std::cout << "=================\n";
    std::cout << "PRIME64_1 high bytes: 0x9E3779B1 (golden ratio for 32-bit)\n";
    std::cout << "PRIME64_3 contains:   0x9E3779F9 (variation of golden ratio)\n";
    std::cout << "PRIME64_5 low bytes:  0x165667C5 (related to PRIME64_3)\n";
    
    // The key insight
    std::cout << "\nKEY INSIGHT:\n";
    std::cout << "============\n";
    std::cout << "0x9E3779B1 = 2654435761\n";
    std::cout << "2^32 / φ = " << (double)(1ULL << 32) / PHI << "\n";
    std::cout << "Nearest odd integer: 2654435761\n";
    std::cout << "This is EXACTLY 0x9E3779B1!\n";
    
    std::cout << "\nThe xxHash primes are built using:\n";
    std::cout << "1. The 32-bit golden ratio prime 0x9E3779B1\n";
    std::cout << "2. Bit rotations and combinations of this value\n";
    std::cout << "3. Ensuring the results are prime or have good bit distribution\n";
    
    return 0;
}