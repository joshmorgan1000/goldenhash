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
 * @file sentiment_demo.cpp
 * @brief Demo of sentiment-based steganographic cipher
 */

#include "goldenhash_sentiment.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace goldenhash;

/**
 * @brief Demonstrate basic sentiment steganography
 */
void demo_basic_stego() {
    std::cout << "\n=== Sentiment-Based Steganography Demo ===\n\n";
    
    // Create key with interesting sentiment mapping
    uint8_t key[8] = {0x42, 0x17, 0x89, 0xAB, 0x3E, 0x7F, 0xC0, 0x55};
    
    std::cout << "Creating cognitive steganography system...\n";
    CognitiveStegano stego(key, 5.0); // 5:1 noise ratio
    
    // Secret message
    std::string secret = "MEET AT MIDNIGHT";
    std::cout << "Secret message: \"" << secret << "\"\n";
    std::cout << "Message length: " << secret.length() << " bytes\n\n";
    
    // Encode with philosophical cover
    std::cout << "Encoding with PHILOSOPHICAL sentiment cover...\n";
    std::string encoded = stego.encode(secret, SentimentType::PHILOSOPHICAL);
    
    std::cout << "\n--- Encoded Stegotext ---\n";
    std::cout << encoded.substr(0, 500) << "...\n";
    std::cout << "\nTotal stegotext length: " << encoded.length() << " characters\n";
    std::cout << "Expansion ratio: " << (encoded.length() / secret.length()) << "x\n";
    
    // Show sentiment flow
    auto flow = stego.get_sentiment_flow();
    std::cout << "\nSentiment flow pattern: ";
    for (auto sentiment : flow) {
        std::cout << static_cast<int>(sentiment) << " ";
    }
    std::cout << "\n";
    
    // Decode
    std::cout << "\nDecoding stegotext...\n";
    std::string decoded = stego.decode(encoded);
    std::cout << "Decoded message: \"" << decoded << "\"\n";
    std::cout << "Decoding " << (decoded == secret ? "SUCCESSFUL" : "FAILED") << "\n";
}

/**
 * @brief Demonstrate honeypot properties
 */
void demo_honeypot_analysis() {
    std::cout << "\n=== Honeypot Analysis Demo ===\n\n";
    
    // Test distinguishability
    std::cout << "Testing adversary's ability to distinguish real vs decoy text...\n";
    double detection_rate = HoneypotAnalyzer::test_distinguishability(100);
    std::cout << "Detection success rate: " << (detection_rate * 100) << "%\n";
    std::cout << "(50% = random guessing, lower is better for us)\n\n";
    
    // Create some stegotext for analysis
    uint8_t key[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    CognitiveStegano stego(key, 8.0);
    
    std::string test_message = "SECRET DATA 12345";
    std::string stego_text = stego.encode(test_message, SentimentType::TECHNICAL);
    
    std::cout << "Analyzing stegotext for patterns...\n";
    auto patterns = HoneypotAnalyzer::analyze_patterns(stego_text);
    for (const auto& finding : patterns) {
        std::cout << "  - " << finding << "\n";
    }
}

/**
 * @brief Show sentiment mapping structure
 */
void demo_sentiment_mapping() {
    std::cout << "\n=== Sentiment S-Box Mapping Demo ===\n\n";
    
    SentimentMapper mapper;
    
    std::cout << "Sampling S-box sentiment profiles:\n\n";
    
    // Show some interesting indices
    std::vector<size_t> sample_indices = {0, 1, 7, 13, 21, 34, 55, 89, 144, 256, 512, 1024, 2047};
    
    for (size_t idx : sample_indices) {
        const auto& profile = mapper.get_profile(idx);
        std::cout << "S-Box[" << std::setw(4) << idx << "]: "
                  << "Primary=" << std::setw(12) << static_cast<int>(profile.primary)
                  << ", Secondary=" << std::setw(12) << static_cast<int>(profile.secondary)
                  << ", Intensity=" << std::fixed << std::setprecision(3) << profile.intensity
                  << ", Coherence=" << profile.coherence
                  << ", Phase=" << profile.ellipticity << "\n";
    }
    
    // Show elliptical pattern
    std::cout << "\nElliptical sentiment journey (first 100 S-boxes):\n";
    for (size_t i = 0; i < 100; i++) {
        const auto& profile = mapper.get_profile(i);
        // Create simple ASCII visualization
        int x = static_cast<int>(10 + 30 * cos(profile.ellipticity));
        int y = static_cast<int>(5 + 5 * sin(profile.ellipticity));
        
        if (i % 10 == 0) {
            std::cout << "\n" << std::setw(3) << i << ": ";
            for (int j = 0; j < x; j++) std::cout << " ";
            std::cout << "*";
        }
    }
    std::cout << "\n";
}

/**
 * @brief Test various attack scenarios
 */
void demo_attack_resistance() {
    std::cout << "\n=== Attack Resistance Demo ===\n\n";
    
    // Test 1: Frequency analysis resistance
    std::cout << "Test 1: Frequency Analysis Resistance\n";
    uint8_t key[8] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    CognitiveStegano stego(key, 10.0);
    
    // Encode same character repeated
    std::string repetitive = std::string(20, 'A');
    std::string encoded = stego.encode(repetitive, SentimentType::TECHNICAL);
    
    std::cout << "Original: " << repetitive << "\n";
    std::cout << "Encoded length: " << encoded.length() << " chars\n";
    std::cout << "First 200 chars: " << encoded.substr(0, 200) << "...\n\n";
    
    // Test 2: Differential analysis
    std::cout << "Test 2: Differential Analysis\n";
    std::string msg1 = "ATTACK AT DAWN";
    std::string msg2 = "ATTACK AT DUSK";
    
    std::string enc1 = stego.encode(msg1, SentimentType::FEAR);
    std::string enc2 = stego.encode(msg2, SentimentType::FEAR);
    
    // Count character differences
    size_t min_len = std::min(enc1.length(), enc2.length());
    size_t diffs = 0;
    for (size_t i = 0; i < min_len; i++) {
        if (enc1[i] != enc2[i]) diffs++;
    }
    
    std::cout << "Message 1: " << msg1 << "\n";
    std::cout << "Message 2: " << msg2 << "\n";
    std::cout << "Input difference: " << 3 << " chars\n";
    std::cout << "Output difference: " << diffs << "/" << min_len 
              << " chars (" << (100.0 * diffs / min_len) << "%)\n";
    std::cout << "Length difference: " << std::abs(static_cast<int>(enc1.length() - enc2.length())) << "\n";
}

/**
 * @brief Main demo program
 */
int main() {
    std::cout << "GoldenHash Sentiment-Based Steganographic Cipher\n";
    std::cout << "===============================================\n";
    std::cout << "\nThis demonstrates a cognitive camouflage system where:\n";
    std::cout << "- Each S-box has emotional/semantic properties\n";
    std::cout << "- Hidden data is embedded in sentiment-coherent noise\n";
    std::cout << "- Elliptical patterns create false trails for attackers\n";
    std::cout << "- The system generates 'almost meaningful' text as honeypot\n";
    
    demo_sentiment_mapping();
    demo_basic_stego();
    demo_honeypot_analysis();
    demo_attack_resistance();
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "The sentiment-based approach creates a unique challenge:\n";
    std::cout << "- Attackers see patterns that seem meaningful but aren't\n";
    std::cout << "- Real data hides among generated 'philosophical' noise\n";
    std::cout << "- Elliptical sentiment flows create false cryptographic trails\n";
    std::cout << "- Neural networks could be trained to generate even more convincing decoys\n";
    std::cout << "\nThis is experimental - not for production use!\n";
    
    return 0;
}