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

#pragma once
/**
 * @file goldenhash_ollama.hpp
 * @brief Ollama integration for real sentiment text generation
 * @details Uses local Ollama models to generate convincing sentiment-based text
 * for the steganographic honeypot system
 */

#include "goldenhash_sentiment.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace goldenhash {

/**
 * @struct OllamaConfig
 * @brief Configuration for Ollama connection
 */
struct OllamaConfig {
    std::string host = "http://localhost:11434";
    std::string model = "llama2";  // Default model
    double temperature = 0.8;
    size_t max_tokens = 200;
    double top_p = 0.9;
    int seed = -1;  // -1 for random
};

/**
 * @class OllamaClient
 * @brief Client for interacting with Ollama API
 */
class OllamaClient {
public:
    /**
     * @brief Constructor
     * @param config Ollama configuration
     */
    explicit OllamaClient(const OllamaConfig& config = OllamaConfig());
    
    /**
     * @brief Destructor
     */
    ~OllamaClient();
    
    /**
     * @brief Generate text from a prompt
     * @param prompt The prompt to send
     * @param stream Whether to stream the response
     * @return Generated text
     */
    std::string generate(const std::string& prompt, bool stream = false);
    
    /**
     * @brief List available models
     * @return Vector of model names
     */
    std::vector<std::string> list_models();
    
    /**
     * @brief Check if Ollama is running
     * @return True if accessible
     */
    bool is_available();
    
private:
    OllamaConfig config_;
    CURL* curl_;
    
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data);
};

/**
 * @class SentimentPromptBuilder
 * @brief Builds prompts for generating sentiment-specific text
 */
class SentimentPromptBuilder {
public:
    /**
     * @brief Build a prompt for a specific sentiment
     * @param sentiment Target sentiment
     * @param context Optional context/seed text
     * @param style Writing style (technical, casual, formal, poetic)
     * @return Prompt string
     */
    static std::string build_prompt(SentimentType sentiment, 
                                   const std::string& context = "",
                                   const std::string& style = "casual");
    
    /**
     * @brief Build honeypot prompt that seems cryptographic
     * @param base_sentiment Underlying emotion
     * @param crypto_terms Whether to include crypto terminology
     * @return Deceptive prompt
     */
    static std::string build_honeypot_prompt(SentimentType base_sentiment,
                                           bool crypto_terms = true);
    
    /**
     * @brief Build elliptical transition prompt
     * @param from_sentiment Starting sentiment
     * @param to_sentiment Target sentiment
     * @param position Position in elliptical cycle (0.0-1.0)
     * @return Transition prompt
     */
    static std::string build_transition_prompt(SentimentType from_sentiment,
                                             SentimentType to_sentiment,
                                             double position);

private:
    static const std::map<SentimentType, std::vector<std::string>> SENTIMENT_DESCRIPTORS;
    static const std::vector<std::string> CRYPTO_TERMS;
    static const std::vector<std::string> TRANSITION_PHRASES;
};

/**
 * @class OllamaSentimentGenerator
 * @brief Enhanced sentiment generator using Ollama models
 */
class OllamaSentimentGenerator : public NeuralSentimentGen {
public:
    /**
     * @brief Constructor
     * @param sentiment Target sentiment type
     * @param model Ollama model to use
     * @param config Ollama configuration
     */
    OllamaSentimentGenerator(SentimentType sentiment,
                           const std::string& model = "llama2",
                           const OllamaConfig& config = OllamaConfig());
    
    /**
     * @brief Generate sentiment-coherent text using Ollama
     * @param seed Initial context
     * @param length Approximate target length
     * @param temperature Creativity (0.0-2.0)
     * @return Generated text
     */
    std::string generate_ollama(const std::string& seed, 
                               size_t length, 
                               double temperature = 0.8);
    
    /**
     * @brief Generate honeypot text that seems significant
     * @param crypto_context Whether to include crypto-like patterns
     * @return Deceptive text
     */
    std::string generate_honeypot(bool crypto_context = true);
    
    /**
     * @brief Generate text with embedded patterns
     * @param pattern Pattern to subtly embed (e.g., fibonacci)
     * @param length Target length
     * @return Text with hidden pattern
     */
    std::string generate_with_pattern(const std::string& pattern, size_t length);
    
protected:
    std::unique_ptr<OllamaClient> ollama_;
    
private:
    std::string model_;
    SentimentType sentiment_;
};

/**
 * @class OllamaCognitiveStegano
 * @brief Enhanced steganography using real LLM-generated text
 */
class OllamaCognitiveStegano : public CognitiveStegano {
public:
    /**
     * @brief Constructor
     * @param key 8-byte cipher key
     * @param model Ollama model to use
     * @param noise_ratio Ratio of decoy to real data
     */
    OllamaCognitiveStegano(const uint8_t key[8],
                          const std::string& model = "llama2",
                          double noise_ratio = 10.0);
    
    /**
     * @brief Encode with LLM-generated cover text
     * @param plaintext Data to hide
     * @param cover_sentiment Overall sentiment theme
     * @param use_honeypot Whether to include honeypot patterns
     * @return Encoded stegotext
     */
    std::string encode_with_llm(const std::string& plaintext,
                               SentimentType cover_sentiment = SentimentType::PHILOSOPHICAL,
                               bool use_honeypot = true);
    
    /**
     * @brief Generate pure honeypot text (no hidden data)
     * @param length Approximate length
     * @param sentiment_flow Vector of sentiments to cycle through
     * @return Honeypot text
     */
    std::string generate_pure_honeypot(size_t length,
                                     const std::vector<SentimentType>& sentiment_flow);
    
    /**
     * @brief Analyze text coherence using LLM
     * @param text Text to analyze
     * @return Coherence score (0.0-1.0)
     */
    double analyze_coherence(const std::string& text);
    
private:
    std::map<SentimentType, std::unique_ptr<OllamaSentimentGenerator>> generators_;
    std::string model_;
};

/**
 * @class AdversarialOllamaTrainer
 * @brief Train system against LLM-based adversaries
 */
class AdversarialOllamaTrainer {
public:
    /**
     * @brief Test if LLM can detect steganography
     * @param detective_model Model to use as adversary
     * @param num_samples Number of test samples
     * @return Detection success rate
     */
    static double test_llm_detection(const std::string& detective_model,
                                   size_t num_samples = 100);
    
    /**
     * @brief Generate adversarial examples
     * @param generator_model Model for generation
     * @param critic_model Model for criticism
     * @param num_rounds Number of refinement rounds
     * @return Refined honeypot examples
     */
    static std::vector<std::string> generate_adversarial_examples(
        const std::string& generator_model,
        const std::string& critic_model,
        size_t num_rounds = 3);
    
    /**
     * @brief Find optimal sentiment flows
     * @param model Model to test with
     * @param num_flows Number of flows to test
     * @return Best sentiment sequences for deception
     */
    static std::vector<std::vector<SentimentType>> find_optimal_flows(
        const std::string& model,
        size_t num_flows = 20);
};

} // namespace goldenhash