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
 * @file cipher_test.cpp
 * @brief Test program for GoldenHash cipher experiments
 */

#include "goldenhash_cipher.hpp"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace goldenhash;

/**
 * @brief Demonstrate basic encryption/decryption
 */
void demo_basic_usage() {
    std::cout << "\nBasic Cipher Usage Demo\n";
    std::cout << "=======================\n";
    
    // Create an 8-byte key
    uint8_t key[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    
    // Analyze the key
    GoldenHashCipher::analyze_key(key);
    
    // Create cipher instance
    GoldenHashCipher cipher_encrypt(key);
    GoldenHashCipher cipher_decrypt(key); // Separate instance for decryption
    
    // Test message
    const char* message = "Hello, GoldenHash Cipher! This is a test of the stream cipher mode.";
    size_t msg_len = strlen(message);
    
    std::cout << "Original message: " << message << "\n";
    std::cout << "Message length: " << msg_len << " bytes\n\n";
    
    // Encrypt
    std::vector<uint8_t> ciphertext(msg_len);
    cipher_encrypt.process(reinterpret_cast<const uint8_t*>(message), msg_len, ciphertext.data());
    
    std::cout << "Ciphertext (hex): ";
    for (size_t i = 0; i < msg_len && i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(ciphertext[i]) << " ";
    }
    if (msg_len > 32) std::cout << "...";
    std::cout << std::dec << "\n\n";
    
    // Decrypt
    std::vector<uint8_t> plaintext(msg_len);
    cipher_decrypt.process(ciphertext.data(), msg_len, plaintext.data());
    
    std::cout << "Decrypted message: ";
    std::cout.write(reinterpret_cast<char*>(plaintext.data()), msg_len);
    std::cout << "\n\n";
    
    // Verify
    bool success = (memcmp(message, plaintext.data(), msg_len) == 0);
    std::cout << "Decryption " << (success ? "SUCCESSFUL" : "FAILED") << "\n";
}

/**
 * @brief Test with different key configurations
 */
void test_key_variations() {
    std::cout << "\nKey Variation Tests\n";
    std::cout << "==================\n";
    
    // Test edge cases
    struct TestCase {
        const char* name;
        uint8_t key[8];
    };
    
    TestCase test_cases[] = {
        {"All zeros", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {"All ones", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
        {"Alternating", {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}},
        {"Sequential", {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}},
        {"High entropy", {0xE3, 0x7B, 0x4C, 0xA9, 0x21, 0xF6, 0x8D, 0x5E}}
    };
    
    uint8_t test_data[32] = {0};
    for (size_t i = 0; i < 32; i++) {
        test_data[i] = i;
    }
    
    for (const auto& test : test_cases) {
        std::cout << "\nTesting key: " << test.name << "\n";
        std::cout << "Key bytes: ";
        for (size_t i = 0; i < 8; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(test.key[i]) << " ";
        }
        std::cout << std::dec << "\n";
        
        GoldenHashCipher cipher(test.key);
        uint8_t output[32];
        cipher.process(test_data, 32, output);
        
        // Show first 16 bytes of output
        std::cout << "Output: ";
        for (size_t i = 0; i < 16; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(output[i]) << " ";
        }
        std::cout << "...\n";
    }
}

/**
 * @brief Main test program
 */
int main(int argc, char* argv[]) {
    std::cout << "GoldenHash Cipher Experimental Analysis\n";
    std::cout << "======================================\n";
    
    // Show configuration
    std::cout << "Configuration:\n";
    std::cout << "  Ciphers in chain: " << GoldenHashCipher::NUM_CIPHERS << "\n";
    std::cout << "  Key size: " << GoldenHashCipher::KEY_SIZE << " bytes\n";
    std::cout << "  Table size bits: " << GoldenHashCipher::TABLE_SIZE_BITS << "\n";
    std::cout << "  Seed bits: " << GoldenHashCipher::SEED_BITS << "\n";
    std::cout << "  Total configurations: " << (1ULL << 16) * GoldenHashCipher::NUM_CIPHERS << "\n";
    
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        // Quick demo only
        demo_basic_usage();
        test_key_variations();
    } else {
        // Full analysis
        demo_basic_usage();
        test_key_variations();
        CipherAnalyzer::run_all_tests();
    }
    
    return 0;
}