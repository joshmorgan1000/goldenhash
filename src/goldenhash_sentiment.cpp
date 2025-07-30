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
 * @file goldenhash_sentiment.cpp
 * @brief Implementation of sentiment-based steganographic system
 */

#include "goldenhash_sentiment.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace goldenhash {

// Sentiment phrase databases
static const std::map<SentimentType, std::vector<std::string>> SENTIMENT_SEEDS = {
    {SentimentType::JOY, {
        "delightful", "wonderful", "amazing", "blessed", "cheerful", 
        "The sunshine brings", "I couldn't be happier", "What a beautiful"
    }},
    {SentimentType::FEAR, {
        "terrifying", "ominous", "dangerous", "threatening", "scary",
        "I'm afraid that", "Something terrible", "The darkness conceals"
    }},
    {SentimentType::TECHNICAL, {
        "algorithm", "implementation", "protocol", "encryption", "analysis",
        "The system requires", "Processing the data", "Security measures indicate"
    }},
    {SentimentType::PHILOSOPHICAL, {
        "existence", "consciousness", "reality", "perception", "meaning",
        "One might consider", "The nature of being", "Truth manifests itself"
    }},
    {SentimentType::FINANCIAL, {
        "investment", "portfolio", "returns", "market", "capital",
        "The quarterly report", "Analysts predict", "Market volatility suggests"
    }},
    // ... would expand for all sentiment types
};

/**
 * @brief Constructor - initialize elliptical sentiment patterns
 */
SentimentMapper::SentimentMapper() {
    init_elliptical_patterns();
    init_language_models();
}

/**
 * @brief Create elliptical emotional journeys across S-boxes
 */
void SentimentMapper::init_elliptical_patterns() {
    // Create multiple elliptical cycles with different periods
    // This creates the "almost coherent" patterns that honeypot seekers might follow
    
    const double PHI = 1.6180339887498948482; // Golden ratio for extra elegance
    
    for (size_t i = 0; i < NUM_SBOXES; i++) {
        SentimentProfile& profile = profiles[i];
        
        // Primary elliptical pattern (major emotional arc)
        double theta1 = (2.0 * M_PI * i) / (NUM_SBOXES / PHI);
        
        // Secondary pattern (creates interference)
        double theta2 = (2.0 * M_PI * i) / (NUM_SBOXES / (PHI * PHI));
        
        // Tertiary pattern (fine-grained noise)
        double theta3 = (2.0 * M_PI * i) / (NUM_SBOXES / (PHI * PHI * PHI));
        
        // Combine patterns with different amplitudes
        double emotional_x = 0.6 * cos(theta1) + 0.3 * cos(theta2) + 0.1 * cos(theta3);
        double emotional_y = 0.6 * sin(theta1) + 0.3 * sin(theta2) + 0.1 * sin(theta3);
        
        // Map to sentiment space
        int primary_idx = static_cast<int>((emotional_x + 1.0) * 16) % static_cast<int>(SentimentType::NUM_SENTIMENTS);
        int secondary_idx = static_cast<int>((emotional_y + 1.0) * 16) % static_cast<int>(SentimentType::NUM_SENTIMENTS);
        
        profile.primary = static_cast<SentimentType>(primary_idx);
        profile.secondary = static_cast<SentimentType>(secondary_idx);
        profile.intensity = 0.5 + 0.5 * sin(theta1 + theta2);
        profile.coherence = 0.3 + 0.4 * cos(theta2 - theta3); // Varies believability
        profile.ellipticity = theta1;
        
        // Create decoy patterns that seem meaningful but aren't
        if (i % 7 == 0) {
            // Every 7th box gets extra "significance" to attract attention
            profile.coherence = std::min(0.9, profile.coherence + 0.3);
        }
        
        // Hidden pattern: boxes at Fibonacci indices get special treatment
        // This creates a false trail for pattern-seekers
        static const std::vector<size_t> fib = {1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597};
        if (std::find(fib.begin(), fib.end(), i) != fib.end()) {
            profile.intensity = 0.95;
        }
    }
}

/**
 * @brief Initialize language models
 */
void SentimentMapper::init_language_models() {
    // In a real implementation, this would load trained models
    // For now, we'll use rule-based generation
    
    for (size_t i = 0; i < NUM_SBOXES; i++) {
        SentimentProfile& profile = profiles[i];
        
        // Assign trigger words based on sentiment combination
        auto primary_it = SENTIMENT_SEEDS.find(profile.primary);
        if (primary_it != SENTIMENT_SEEDS.end() && !primary_it->second.empty()) {
            size_t word_idx = i % primary_it->second.size();
            profile.trigger_words.push_back(primary_it->second[word_idx]);
        }
        
        // Create plausible but ultimately meaningless phrases
        std::stringstream decoy;
        decoy << "The " << static_cast<int>(profile.primary) 
              << " aspect reveals " << static_cast<int>(profile.secondary);
        profile.decoy_phrases.push_back(decoy.str());
    }
}

/**
 * @brief Get sentiment profile for index
 */
const SentimentProfile& SentimentMapper::get_profile(size_t index) const {
    return profiles[index % NUM_SBOXES];
}

/**
 * @brief Generate decoy text
 */
std::string SentimentMapper::generate_decoy(const SentimentProfile& profile, size_t length) const {
    std::stringstream output;
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Get words for this sentiment
    auto it = SENTIMENT_SEEDS.find(profile.primary);
    if (it == SENTIMENT_SEEDS.end() || it->second.empty()) {
        // Fallback to generic text
        output << "The process continues with careful consideration. ";
    } else {
        const auto& words = it->second;
        std::uniform_int_distribution<> dist(0, words.size() - 1);
        
        // Generate semi-coherent text
        while (output.str().length() < length) {
            output << words[dist(gen)] << " ";
            
            // Add connecting phrases for realism
            if (output.str().length() % 50 < 10) {
                output << "which leads to ";
            } else if (output.str().length() % 70 < 10) {
                output << "suggesting that ";
            }
        }
    }
    
    return output.str().substr(0, length);
}

/**
 * @brief Constructor for cognitive steganography
 */
CognitiveStegano::CognitiveStegano(const uint8_t key[8], double noise_ratio) 
    : cipher(std::make_unique<GoldenHashCipher>(key)),
      mapper(std::make_unique<SentimentMapper>()),
      noise_ratio(noise_ratio) {
}

/**
 * @brief Encode plaintext into sentiment stream
 */
std::string CognitiveStegano::encode(const std::string& plaintext, SentimentType cover_sentiment) {
    std::stringstream output;
    
    // Convert plaintext to bytes
    std::vector<uint8_t> plain_bytes(plaintext.begin(), plaintext.end());
    
    // Encrypt the plaintext
    std::vector<uint8_t> encrypted(plain_bytes.size());
    cipher->process(plain_bytes.data(), plain_bytes.size(), encrypted.data());
    
    // Get S-box configuration from cipher key
    // This determines our sentiment flow pattern
    const uint8_t* key_ptr = reinterpret_cast<const uint8_t*>(&cipher);
    std::vector<size_t> sbox_indices;
    for (size_t i = 0; i < 4; i++) {
        uint16_t subkey = (key_ptr[i*2] << 8) | key_ptr[i*2 + 1];
        size_t table_index = (subkey >> 5) & 0x7FF;
        sbox_indices.push_back(table_index);
    }
    
    // Generate cover text with embedded data
    size_t byte_idx = 0;
    size_t bit_idx = 0;
    
    // Start with plausible introduction based on cover sentiment
    const auto& cover_profile = mapper->get_profile(static_cast<size_t>(cover_sentiment));
    output << mapper->generate_decoy(cover_profile, 50 + rand() % 100) << "\n\n";
    
    // Embed data using sentiment transitions
    while (byte_idx < encrypted.size()) {
        // Select S-box based on position in stream
        size_t sbox_idx = sbox_indices[byte_idx % sbox_indices.size()];
        const auto& profile = mapper->get_profile(sbox_idx);
        
        // Generate noise segments
        for (int n = 0; n < noise_ratio; n++) {
            // Create decoy text that follows elliptical pattern
            size_t decoy_length = 20 + (rand() % 80);
            output << mapper->generate_decoy(profile, decoy_length) << " ";
            
            // Occasionally inject false patterns to mislead
            if (rand() % 10 == 0) {
                output << "[" << std::hex << (rand() % 256) << "] ";
            }
        }
        
        // Embed actual data bit
        if (byte_idx < encrypted.size()) {
            uint8_t byte = encrypted[byte_idx];
            uint8_t bit = (byte >> bit_idx) & 1;
            
            // Use sentiment intensity to encode bit
            if (bit) {
                output << "Indeed, ";
            } else {
                output << "However, ";
            }
            
            bit_idx++;
            if (bit_idx >= 8) {
                bit_idx = 0;
                byte_idx++;
            }
        }
        
        // Add paragraph breaks for readability
        if (output.str().length() % 500 < 50) {
            output << "\n\n";
        }
    }
    
    // Concluding decoy text
    output << "\n\n" << mapper->generate_decoy(cover_profile, 100 + rand() % 100);
    
    return output.str();
}

/**
 * @brief Decode hidden data from sentiment stream
 */
std::string CognitiveStegano::decode(const std::string& stego_text) {
    std::vector<uint8_t> extracted_bits;
    std::vector<uint8_t> extracted_bytes;
    
    // Look for our encoding markers
    size_t pos = 0;
    uint8_t current_byte = 0;
    size_t bit_count = 0;
    
    while (pos < stego_text.length()) {
        // Find "Indeed, " (1) or "However, " (0)
        size_t indeed_pos = stego_text.find("Indeed, ", pos);
        size_t however_pos = stego_text.find("However, ", pos);
        
        size_t next_pos = std::string::npos;
        uint8_t bit = 0;
        
        if (indeed_pos != std::string::npos && 
            (however_pos == std::string::npos || indeed_pos < however_pos)) {
            bit = 1;
            next_pos = indeed_pos + 8;
        } else if (however_pos != std::string::npos) {
            bit = 0;
            next_pos = however_pos + 9;
        } else {
            break; // No more encoded bits
        }
        
        current_byte |= (bit << (bit_count % 8));
        bit_count++;
        
        if (bit_count % 8 == 0) {
            extracted_bytes.push_back(current_byte);
            current_byte = 0;
        }
        
        pos = next_pos;
    }
    
    // Decrypt the extracted bytes
    std::vector<uint8_t> decrypted(extracted_bytes.size());
    cipher->process(extracted_bytes.data(), extracted_bytes.size(), decrypted.data());
    
    return std::string(decrypted.begin(), decrypted.end());
}

/**
 * @brief Get sentiment flow for analysis
 */
std::vector<SentimentType> CognitiveStegano::get_sentiment_flow() const {
    std::vector<SentimentType> flow;
    
    // Extract S-box indices from cipher configuration
    const uint8_t* key_ptr = reinterpret_cast<const uint8_t*>(&cipher);
    for (size_t i = 0; i < 4; i++) {
        uint16_t subkey = (key_ptr[i*2] << 8) | key_ptr[i*2 + 1];
        size_t table_index = (subkey >> 5) & 0x7FF;
        const auto& profile = mapper->get_profile(table_index);
        flow.push_back(profile.primary);
        flow.push_back(profile.secondary);
    }
    
    return flow;
}

/**
 * @brief Test distinguishability
 */
double HoneypotAnalyzer::test_distinguishability(size_t num_samples) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    size_t correct_guesses = 0;
    
    for (size_t i = 0; i < num_samples; i++) {
        // Generate random key
        uint8_t key[8];
        for (size_t j = 0; j < 8; j++) {
            key[j] = dist(gen);
        }
        
        CognitiveStegano stego(key);
        
        // Half real, half pure decoy
        bool is_real = (i % 2 == 0);
        std::string test_text;
        
        if (is_real) {
            test_text = stego.encode("This is hidden data", SentimentType::PHILOSOPHICAL);
        } else {
            // Pure decoy
            SentimentMapper mapper;
            test_text = mapper.generate_decoy(mapper.get_profile(i), 1000);
        }
        
        // Simple heuristic: look for encoding patterns
        size_t indeed_count = 0;
        size_t however_count = 0;
        size_t pos = 0;
        while ((pos = test_text.find("Indeed, ", pos)) != std::string::npos) {
            indeed_count++;
            pos += 8;
        }
        pos = 0;
        while ((pos = test_text.find("However, ", pos)) != std::string::npos) {
            however_count++;
            pos += 9;
        }
        
        // Guess based on pattern regularity
        bool guess_real = (indeed_count + however_count) > 5;
        
        if (guess_real == is_real) {
            correct_guesses++;
        }
    }
    
    return static_cast<double>(correct_guesses) / num_samples;
}

/**
 * @brief Analyze patterns in stegotext
 */
std::vector<std::string> HoneypotAnalyzer::analyze_patterns(const std::string& stego_text) {
    std::vector<std::string> findings;
    
    // Check for encoding markers
    size_t indeed_count = 0;
    size_t however_count = 0;
    size_t pos = 0;
    
    while ((pos = stego_text.find("Indeed, ", pos)) != std::string::npos) {
        indeed_count++;
        pos += 8;
    }
    pos = 0;
    while ((pos = stego_text.find("However, ", pos)) != std::string::npos) {
        however_count++;
        pos += 9;
    }
    
    if (indeed_count + however_count > 0) {
        std::stringstream ss;
        ss << "Found " << indeed_count << " 'Indeed' and " 
           << however_count << " 'However' markers";
        findings.push_back(ss.str());
    }
    
    // Check for hex patterns (false flags)
    pos = 0;
    size_t hex_count = 0;
    while ((pos = stego_text.find("[", pos)) != std::string::npos) {
        size_t end = stego_text.find("]", pos);
        if (end != std::string::npos && end - pos < 10) {
            hex_count++;
        }
        pos++;
    }
    
    if (hex_count > 0) {
        std::stringstream ss;
        ss << "Found " << hex_count << " potential hex markers (likely decoys)";
        findings.push_back(ss.str());
    }
    
    // Analyze sentiment transitions
    // (Would use actual NLP in production)
    findings.push_back("Sentiment flow appears elliptical with period ~" + 
                      std::to_string(stego_text.length() / 10));
    
    return findings;
}

/**
 * @brief Constructor for neural sentiment generator
 */
NeuralSentimentGen::NeuralSentimentGen(SentimentType sentiment, const std::string& model_size)
    : sentiment(sentiment) {
    // Initialize vocabulary with basic words
    vocabulary = {"the", "a", "an", "is", "was", "are", "were", "been", "being", "have", "has", "had",
                  "do", "does", "did", "will", "would", "could", "should", "may", "might", "must"};
}

/**
 * @brief Generate text using simple markov chains
 */
std::string NeuralSentimentGen::generate(const std::string& seed, size_t length, double temperature) {
    std::stringstream output;
    output << seed;
    
    // Simple generation based on sentiment
    auto it = SENTIMENT_SEEDS.find(sentiment);
    if (it != SENTIMENT_SEEDS.end() && !it->second.empty()) {
        const auto& words = it->second;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, words.size() - 1);
        
        while (output.str().length() < length) {
            output << " " << words[dist(gen)];
            
            // Add variety
            if (rand() % 5 == 0) {
                output << " " << vocabulary[rand() % vocabulary.size()];
            }
        }
    }
    
    return output.str().substr(0, length);
}

/**
 * @brief Placeholder for fine-tuning
 */
void NeuralSentimentGen::fine_tune(const std::vector<std::string>& examples) {
    // In a real implementation, this would update the model
    // For now, just add to vocabulary
    for (const auto& example : examples) {
        std::istringstream iss(example);
        std::string word;
        while (iss >> word) {
            if (word.length() > 3) {
                vocabulary.push_back(word);
            }
        }
    }
}

} // namespace goldenhash