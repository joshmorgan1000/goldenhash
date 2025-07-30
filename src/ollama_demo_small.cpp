/**
 * @file ollama_demo_small.cpp
 * @brief Simplified demo showing key honeypot features
 */

#include "goldenhash_ollama.hpp"
#include <iostream>

using namespace goldenhash;

int main() {
    std::cout << "=== GoldenHash Honeypot Demo with Real LLM ===\n\n";
    
    // Check Ollama
    OllamaClient client;
    if (!client.is_available()) {
        std::cout << "Ollama not running - please start with: ollama serve\n";
        return 1;
    }
    
    // Generate a honeypot text
    std::cout << "1. Generating cryptographic honeypot text:\n";
    std::cout << "   (This looks technical but is meaningless)\n\n";
    
    OllamaConfig config;
    config.model = "mistral-nemo:latest";
    config.temperature = 0.9;
    config.max_tokens = 150;
    
    OllamaSentimentGenerator generator(SentimentType::TECHNICAL, config.model, config);
    std::string honeypot = generator.generate_honeypot(true);
    std::cout << honeypot << "\n\n";
    
    // Show sentiment transition
    std::cout << "2. Elliptical sentiment transition (Joy → Fear):\n\n";
    
    std::string transition_prompt = SentimentPromptBuilder::build_transition_prompt(
        SentimentType::JOY, SentimentType::FEAR, 0.5);
    
    OllamaClient transition_client(config);
    std::string transition = transition_client.generate(transition_prompt, false);
    std::cout << transition << "\n\n";
    
    // Demonstrate steganography
    std::cout << "3. Hiding message in sentiment-based text:\n\n";
    
    uint8_t key[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    OllamaCognitiveStegano stego(key, config.model, 2.0); // Low noise for demo
    
    std::string secret = "HELLO";
    std::cout << "Secret: \"" << secret << "\"\n";
    std::cout << "Encoding...\n\n";
    
    // Use simpler encode (not LLM version for speed)
    std::string encoded = stego.encode(secret, SentimentType::PHILOSOPHICAL);
    std::cout << "Preview (first 300 chars):\n";
    std::cout << encoded.substr(0, 300) << "...\n\n";
    
    std::cout << "=== Key Insights ===\n";
    std::cout << "• Each of 2048 S-boxes has a sentiment profile\n";
    std::cout << "• LLM generates convincing cover text\n";
    std::cout << "• Elliptical patterns create false cryptographic trails\n";
    std::cout << "• Attackers see meaningful patterns that aren't real\n";
    std::cout << "• Real data hides in AI-generated philosophical noise\n";
    
    return 0;
}