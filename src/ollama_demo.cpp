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
 * @file ollama_demo.cpp
 * @brief Demo of Ollama-powered sentiment steganography
 */

#include "goldenhash_ollama.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace goldenhash;

/**
 * @brief Check Ollama connection and list models
 */
void check_ollama_setup() {
    std::cout << "\n=== Ollama Setup Check ===\n\n";
    
    OllamaClient client;
    
    if (!client.is_available()) {
        std::cout << "ERROR: Cannot connect to Ollama at localhost:11434\n";
        std::cout << "Please ensure Ollama is running: ollama serve\n";
        return;
    }
    
    std::cout << "✓ Ollama is running\n\n";
    std::cout << "Available models:\n";
    
    auto models = client.list_models();
    if (models.empty()) {
        std::cout << "  No models found. Pull a model with: ollama pull llama2\n";
    } else {
        for (const auto& model : models) {
            std::cout << "  - " << model << "\n";
        }
    }
}

/**
 * @brief Demo sentiment text generation
 */
void demo_sentiment_generation() {
    std::cout << "\n=== LLM Sentiment Generation Demo ===\n\n";
    
    // Check if Ollama is available
    OllamaClient test_client;
    if (!test_client.is_available()) {
        std::cout << "Skipping LLM demo - Ollama not available\n";
        return;
    }
    
    // Test different sentiments
    std::vector<std::pair<SentimentType, std::string>> test_sentiments = {
        {SentimentType::JOY, "Joy"},
        {SentimentType::FEAR, "Fear"},
        {SentimentType::PHILOSOPHICAL, "Philosophical"},
        {SentimentType::TECHNICAL, "Technical"},
        {SentimentType::MYSTERY, "Mystery"}
    };
    
    OllamaConfig config;
    config.model = "mistral-nemo:latest";  // Using your available model
    config.temperature = 0.8;
    config.max_tokens = 100;
    
    for (const auto& [sentiment, name] : test_sentiments) {
        std::cout << "Generating " << name << " text:\n";
        
        OllamaSentimentGenerator generator(sentiment, config.model, config);
        
        auto start = std::chrono::high_resolution_clock::now();
        std::string generated = generator.generate_ollama("", 100, 0.8);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "  " << generated << "\n";
        std::cout << "  (Generated in " << duration.count() << "ms)\n\n";
    }
}

/**
 * @brief Demo honeypot generation
 */
void demo_honeypot_generation() {
    std::cout << "\n=== Honeypot Text Generation Demo ===\n\n";
    
    OllamaClient test_client;
    if (!test_client.is_available()) {
        std::cout << "Skipping honeypot demo - Ollama not available\n";
        return;
    }
    
    OllamaConfig config;
    config.model = "mistral-nemo:latest";
    config.temperature = 0.9;  // Higher creativity for honeypots
    
    std::cout << "Generating cryptographic honeypot texts:\n\n";
    
    // Generate honeypots with different base sentiments
    std::vector<SentimentType> honeypot_sentiments = {
        SentimentType::TECHNICAL,
        SentimentType::MYSTERY,
        SentimentType::PHILOSOPHICAL
    };
    
    for (auto sentiment : honeypot_sentiments) {
        OllamaSentimentGenerator generator(sentiment, config.model, config);
        
        std::cout << "Honeypot with " << static_cast<int>(sentiment) << " undertone:\n";
        std::string honeypot = generator.generate_honeypot(true);
        std::cout << "  " << honeypot << "\n\n";
    }
}

/**
 * @brief Demo full steganographic encoding with LLM
 */
void demo_llm_steganography() {
    std::cout << "\n=== LLM-Powered Steganography Demo ===\n\n";
    
    OllamaClient test_client;
    if (!test_client.is_available()) {
        std::cout << "Skipping steganography demo - Ollama not available\n";
        return;
    }
    
    // Create key
    uint8_t key[8] = {0x13, 0x37, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    
    // Choose model (use a smaller/faster one if available)
    std::string model = "mistral-nemo:latest";  // Using available model
    
    std::cout << "Using model: " << model << "\n";
    std::cout << "Creating LLM-powered steganographic system...\n\n";
    
    OllamaCognitiveStegano stego(key, model, 3.0);  // Lower noise ratio for demo
    
    // Secret message
    std::string secret = "GOLDENHASH";
    std::cout << "Secret message: \"" << secret << "\"\n";
    std::cout << "Encoding with LLM-generated cover text...\n\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string encoded = stego.encode_with_llm(secret, SentimentType::PHILOSOPHICAL, true);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    std::cout << "--- Generated Stegotext Preview ---\n";
    std::cout << encoded.substr(0, 500) << "...\n\n";
    
    std::cout << "Statistics:\n";
    std::cout << "  Generation time: " << duration.count() << " seconds\n";
    std::cout << "  Total length: " << encoded.length() << " characters\n";
    std::cout << "  Expansion ratio: " << (encoded.length() / secret.length()) << "x\n";
    
    // Test coherence
    std::cout << "\nAnalyzing coherence...\n";
    double coherence = stego.analyze_coherence(encoded);
    std::cout << "  Coherence score: " << std::fixed << std::setprecision(2) 
              << (coherence * 100) << "%\n";
}

/**
 * @brief Demo sentiment flow transitions
 */
void demo_sentiment_transitions() {
    std::cout << "\n=== Sentiment Flow Transitions Demo ===\n\n";
    
    OllamaClient test_client;
    if (!test_client.is_available()) {
        std::cout << "Skipping transitions demo - Ollama not available\n";
        return;
    }
    
    // Create an elliptical sentiment journey
    std::vector<SentimentType> journey = {
        SentimentType::JOY,
        SentimentType::ANTICIPATION,
        SentimentType::SURPRISE,
        SentimentType::CONFUSION,
        SentimentType::MYSTERY,
        SentimentType::FEAR,
        SentimentType::SADNESS,
        SentimentType::TRUST,
        SentimentType::JOY  // Complete the cycle
    };
    
    std::cout << "Generating elliptical sentiment journey:\n\n";
    
    OllamaConfig config;
    config.model = "mistral-nemo:latest";
    config.temperature = 0.7;
    config.max_tokens = 80;
    
    for (size_t i = 0; i < journey.size() - 1; i++) {
        SentimentType from = journey[i];
        SentimentType to = journey[i + 1];
        double position = static_cast<double>(i) / (journey.size() - 1);
        
        std::cout << "Transition " << (i + 1) << " (" 
                  << static_cast<int>(from) << " → " 
                  << static_cast<int>(to) << "):\n";
        
        std::string prompt = SentimentPromptBuilder::build_transition_prompt(
            from, to, position);
        
        OllamaClient client(config);
        std::string transition = client.generate(prompt, false);
        
        std::cout << "  " << transition << "\n\n";
    }
}

/**
 * @brief Main demo program
 */
int main(int argc, char* argv[]) {
    std::cout << "GoldenHash LLM-Powered Sentiment Steganography\n";
    std::cout << "==============================================\n";
    std::cout << "\nThis demonstrates using Ollama LLMs to generate\n";
    std::cout << "convincing sentiment-based cover text for steganography.\n";
    
    // Check Ollama setup first
    check_ollama_setup();
    
    OllamaClient test_client;
    if (!test_client.is_available()) {
        std::cout << "\n⚠️  Ollama is not running. Please start it with:\n";
        std::cout << "   ollama serve\n\n";
        std::cout << "Then pull a model:\n";
        std::cout << "   ollama pull llama2\n\n";
        return 1;
    }
    
    // Run demos
    if (argc > 1 && std::string(argv[1]) == "--quick") {
        // Quick demo - just show one example
        demo_sentiment_generation();
    } else {
        // Full demo suite
        demo_sentiment_generation();
        demo_honeypot_generation();
        demo_sentiment_transitions();
        demo_llm_steganography();
    }
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "LLM integration provides:\n";
    std::cout << "- Natural, coherent sentiment-based text\n";
    std::cout << "- Convincing honeypot patterns\n";
    std::cout << "- Smooth sentiment transitions\n";
    std::cout << "- Higher quality camouflage for hidden data\n";
    std::cout << "\nThe combination of GoldenHash cipher + LLM-generated cover\n";
    std::cout << "creates a cognitive challenge for adversaries!\n";
    
    return 0;
}