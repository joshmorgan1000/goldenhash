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
 * @file goldenhash_ollama.cpp
 * @brief Implementation of Ollama integration for sentiment generation
 */

#include "goldenhash_ollama.hpp"
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>

namespace goldenhash {

// Sentiment descriptors for prompt building
const std::map<SentimentType, std::vector<std::string>> SentimentPromptBuilder::SENTIMENT_DESCRIPTORS = {
    {SentimentType::JOY, {"happy", "joyful", "elated", "cheerful", "delighted", "ecstatic"}},
    {SentimentType::FEAR, {"afraid", "terrified", "anxious", "worried", "frightened", "alarmed"}},
    {SentimentType::ANGER, {"angry", "furious", "enraged", "irritated", "frustrated", "livid"}},
    {SentimentType::SADNESS, {"sad", "melancholy", "depressed", "sorrowful", "gloomy", "dejected"}},
    {SentimentType::TRUST, {"trusting", "confident", "secure", "assured", "believing", "faithful"}},
    {SentimentType::DISGUST, {"disgusted", "repulsed", "revolted", "sickened", "appalled"}},
    {SentimentType::SURPRISE, {"surprised", "astonished", "amazed", "shocked", "startled"}},
    {SentimentType::ANTICIPATION, {"anticipating", "expecting", "eager", "hopeful", "excited"}},
    {SentimentType::PHILOSOPHICAL, {"contemplative", "thoughtful", "reflective", "philosophical", "pondering"}},
    {SentimentType::TECHNICAL, {"analytical", "technical", "systematic", "methodical", "precise"}},
    {SentimentType::FINANCIAL, {"economic", "financial", "monetary", "fiscal", "commercial"}},
    {SentimentType::MYSTERY, {"mysterious", "enigmatic", "cryptic", "puzzling", "arcane"}}
};

const std::vector<std::string> SentimentPromptBuilder::CRYPTO_TERMS = {
    "entropy", "cipher", "hash", "key", "algorithm", "protocol", "signature",
    "modulus", "prime", "elliptic", "curve", "field", "group", "generator",
    "initialization vector", "salt", "nonce", "padding", "block", "stream"
};

const std::vector<std::string> SentimentPromptBuilder::TRANSITION_PHRASES = {
    "which leads us to consider", "transitioning into", "evolving towards",
    "shifting perspective to", "gradually becoming", "transforming into"
};

/**
 * @brief CURL write callback
 */
size_t OllamaClient::write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

/**
 * @brief Constructor
 */
OllamaClient::OllamaClient(const OllamaConfig& config) : config_(config) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
}

/**
 * @brief Destructor
 */
OllamaClient::~OllamaClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

/**
 * @brief Check if Ollama is available
 */
bool OllamaClient::is_available() {
    if (!curl_) return false;
    
    std::string response;
    std::string url = config_.host + "/api/tags";
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl_);
    return res == CURLE_OK;
}

/**
 * @brief Generate text from prompt
 */
std::string OllamaClient::generate(const std::string& prompt, bool stream) {
    if (!curl_) return "Error: CURL not initialized";
    
    std::string response;
    std::string url = config_.host + "/api/generate";
    
    // Build JSON request
    nlohmann::json request;
    request["model"] = config_.model;
    request["prompt"] = prompt;
    request["stream"] = stream;
    request["options"]["temperature"] = config_.temperature;
    request["options"]["num_predict"] = static_cast<int>(config_.max_tokens);
    request["options"]["top_p"] = config_.top_p;
    if (config_.seed >= 0) {
        request["options"]["seed"] = config_.seed;
    }
    
    std::string json_str = request.dump();
    
    // Setup CURL
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl_);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        return "Error: " + std::string(curl_easy_strerror(res));
    }
    
    // Parse response
    std::istringstream response_stream(response);
    
    // Handle streaming response (multiple JSON objects)
    std::string full_response;
    std::string line;
    while (std::getline(response_stream, line)) {
        if (line.empty()) continue;
        
        try {
            nlohmann::json json_response = nlohmann::json::parse(line);
            if (json_response.contains("response")) {
                full_response += json_response["response"].get<std::string>();
            }
        } catch (const nlohmann::json::exception& e) {
            // Skip malformed lines
        }
    }
    
    return full_response.empty() ? response : full_response;
}

/**
 * @brief List available models
 */
std::vector<std::string> OllamaClient::list_models() {
    std::vector<std::string> models;
    if (!curl_) return models;
    
    std::string response;
    std::string url = config_.host + "/api/tags";
    
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl_);
    if (res != CURLE_OK) {
        return models;
    }
    
    // Parse JSON response
    try {
        nlohmann::json root = nlohmann::json::parse(response);
        if (root.contains("models") && root["models"].is_array()) {
            for (const auto& model : root["models"]) {
                if (model.contains("name")) {
                    models.push_back(model["name"].get<std::string>());
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        // Return empty models list on parse error
    }
    
    return models;
}

/**
 * @brief Build sentiment prompt
 */
std::string SentimentPromptBuilder::build_prompt(SentimentType sentiment,
                                                const std::string& context,
                                                const std::string& style) {
    std::stringstream prompt;
    
    auto it = SENTIMENT_DESCRIPTORS.find(sentiment);
    if (it == SENTIMENT_DESCRIPTORS.end() || it->second.empty()) {
        prompt << "Write a short " << style << " text. ";
    } else {
        const auto& descriptors = it->second;
        std::string descriptor = descriptors[rand() % descriptors.size()];
        
        prompt << "Write a short " << style << " text that conveys a " 
               << descriptor << " sentiment. ";
    }
    
    if (!context.empty()) {
        prompt << "Continue from this context: \"" << context << "\". ";
    }
    
    prompt << "Be natural and avoid mentioning emotions directly. "
           << "Keep it under 100 words.";
    
    return prompt.str();
}

/**
 * @brief Build honeypot prompt
 */
std::string SentimentPromptBuilder::build_honeypot_prompt(SentimentType base_sentiment,
                                                         bool crypto_terms) {
    std::stringstream prompt;
    
    prompt << "Write a technical analysis that seems important but is actually meaningless. ";
    
    if (crypto_terms && !CRYPTO_TERMS.empty()) {
        prompt << "Use terms like: ";
        for (size_t i = 0; i < 3 && i < CRYPTO_TERMS.size(); i++) {
            prompt << CRYPTO_TERMS[rand() % CRYPTO_TERMS.size()];
            if (i < 2) prompt << ", ";
        }
        prompt << ". ";
    }
    
    auto it = SENTIMENT_DESCRIPTORS.find(base_sentiment);
    if (it != SENTIMENT_DESCRIPTORS.end() && !it->second.empty()) {
        prompt << "Subtly convey a " << it->second[0] << " undertone. ";
    }
    
    prompt << "Make it sound like you're revealing something significant. "
           << "Use numbers and technical jargon. Keep it vague but intriguing.";
    
    return prompt.str();
}

/**
 * @brief Build transition prompt
 */
std::string SentimentPromptBuilder::build_transition_prompt(SentimentType from_sentiment,
                                                           SentimentType to_sentiment,
                                                           double position) {
    std::stringstream prompt;
    
    auto from_it = SENTIMENT_DESCRIPTORS.find(from_sentiment);
    auto to_it = SENTIMENT_DESCRIPTORS.find(to_sentiment);
    
    std::string from_desc = (from_it != SENTIMENT_DESCRIPTORS.end() && !from_it->second.empty()) 
                           ? from_it->second[0] : "neutral";
    std::string to_desc = (to_it != SENTIMENT_DESCRIPTORS.end() && !to_it->second.empty()) 
                         ? to_it->second[0] : "neutral";
    
    prompt << "Write a paragraph that transitions from a " << from_desc 
           << " mood to a " << to_desc << " mood. ";
    
    if (position < 0.3) {
        prompt << "Start the transition subtly. ";
    } else if (position > 0.7) {
        prompt << "Complete the transition decisively. ";
    } else {
        prompt << "Make the transition gradual and natural. ";
    }
    
    if (!TRANSITION_PHRASES.empty()) {
        prompt << "Use a phrase like '" 
               << TRANSITION_PHRASES[rand() % TRANSITION_PHRASES.size()] 
               << "' if appropriate. ";
    }
    
    prompt << "Keep it under 80 words.";
    
    return prompt.str();
}

/**
 * @brief Constructor for Ollama sentiment generator
 */
OllamaSentimentGenerator::OllamaSentimentGenerator(SentimentType sentiment,
                                                 const std::string& model,
                                                 const OllamaConfig& config)
    : NeuralSentimentGen(sentiment, "ollama"),
      ollama_(std::make_unique<OllamaClient>(config)),
      model_(model),
      sentiment_(sentiment) {
    
    // Update config with model
    OllamaConfig updated_config = config;
    updated_config.model = model;
    ollama_ = std::make_unique<OllamaClient>(updated_config);
}

/**
 * @brief Generate text using Ollama
 */
std::string OllamaSentimentGenerator::generate_ollama(const std::string& seed,
                                                    size_t length,
                                                    double temperature) {
    if (!ollama_->is_available()) {
        // Fallback to base class generation
        return generate(seed, length, temperature);
    }
    
    // Build prompt
    std::string prompt = SentimentPromptBuilder::build_prompt(sentiment_, seed, "conversational");
    
    // Generate text
    std::string generated = ollama_->generate(prompt, false);
    
    // Trim to approximate length
    if (generated.length() > length) {
        // Find a good breaking point
        size_t break_point = generated.find(". ", length * 0.8);
        if (break_point != std::string::npos && break_point < length * 1.2) {
            generated = generated.substr(0, break_point + 1);
        } else {
            generated = generated.substr(0, length);
        }
    }
    
    return generated;
}

/**
 * @brief Generate honeypot text
 */
std::string OllamaSentimentGenerator::generate_honeypot(bool crypto_context) {
    if (!ollama_->is_available()) {
        return "The entropy analysis reveals patterns in the modular arithmetic...";
    }
    
    std::string prompt = SentimentPromptBuilder::build_honeypot_prompt(sentiment_, crypto_context);
    return ollama_->generate(prompt, false);
}

/**
 * @brief Generate text with embedded pattern
 */
std::string OllamaSentimentGenerator::generate_with_pattern(const std::string& pattern, 
                                                           size_t length) {
    if (!ollama_->is_available()) {
        return generate("Pattern: " + pattern, length, 1.0);
    }
    
    std::stringstream prompt;
    prompt << "Write a text that subtly incorporates the pattern '" << pattern 
           << "' without directly mentioning it. ";
    prompt << "The pattern should emerge naturally from the content. ";
    prompt << "Convey a " << static_cast<int>(sentiment_) << " sentiment. ";
    prompt << "Make it approximately " << length << " characters.";
    
    return ollama_->generate(prompt.str(), false);
}

/**
 * @brief Constructor for Ollama cognitive stegano
 */
OllamaCognitiveStegano::OllamaCognitiveStegano(const uint8_t key[8],
                                               const std::string& model,
                                               double noise_ratio)
    : CognitiveStegano(key, noise_ratio), model_(model) {
    
    // Initialize generators for common sentiments
    OllamaConfig config;
    config.model = model;
    config.temperature = 0.8;
    
    for (int i = 0; i < static_cast<int>(SentimentType::NUM_SENTIMENTS); i++) {
        SentimentType sentiment = static_cast<SentimentType>(i);
        generators_[sentiment] = std::make_unique<OllamaSentimentGenerator>(
            sentiment, model, config);
    }
}

/**
 * @brief Encode with LLM-generated text
 */
std::string OllamaCognitiveStegano::encode_with_llm(const std::string& plaintext,
                                                   SentimentType cover_sentiment,
                                                   bool use_honeypot) {
    std::stringstream output;
    
    // Convert plaintext to bytes and encrypt
    std::vector<uint8_t> plain_bytes(plaintext.begin(), plaintext.end());
    std::vector<uint8_t> encrypted(plain_bytes.size());
    cipher->process(plain_bytes.data(), plain_bytes.size(), encrypted.data());
    
    // Get sentiment flow from cipher configuration
    auto sentiment_flow = get_sentiment_flow();
    
    // Generate introduction using LLM
    auto& intro_gen = generators_[cover_sentiment];
    output << intro_gen->generate_ollama("", 150 + rand() % 100) << "\n\n";
    
    // Embed data with LLM-generated noise
    size_t byte_idx = 0;
    size_t bit_idx = 0;
    size_t sentiment_idx = 0;
    
    while (byte_idx < encrypted.size()) {
        // Cycle through sentiment flow
        SentimentType current_sentiment = sentiment_flow[sentiment_idx % sentiment_flow.size()];
        auto& generator = generators_[current_sentiment];
        
        // Generate noise segments
        for (int n = 0; n < noise_ratio; n++) {
            if (use_honeypot && rand() % 3 == 0) {
                // Insert honeypot text
                output << generator->generate_honeypot(true) << " ";
            } else {
                // Normal sentiment text
                output << generator->generate_ollama("", 50 + rand() % 100) << " ";
            }
        }
        
        // Embed actual data bit (using more subtle encoding)
        if (byte_idx < encrypted.size()) {
            uint8_t byte = encrypted[byte_idx];
            uint8_t bit = (byte >> bit_idx) & 1;
            
            // Use different encoding based on sentiment
            if (current_sentiment == SentimentType::TECHNICAL) {
                // Technical encoding
                if (bit) {
                    output << "Furthermore, analysis shows ";
                } else {
                    output << "Alternatively, we observe ";
                }
            } else if (current_sentiment == SentimentType::PHILOSOPHICAL) {
                // Philosophical encoding
                if (bit) {
                    output << "Indeed, one might say ";
                } else {
                    output << "However, consider that ";
                }
            } else {
                // Generic encoding
                if (bit) {
                    output << "Moreover, ";
                } else {
                    output << "Nevertheless, ";
                }
            }
            
            bit_idx++;
            if (bit_idx >= 8) {
                bit_idx = 0;
                byte_idx++;
                sentiment_idx++;
            }
        }
        
        // Add transitions between sentiments
        if (sentiment_idx % 4 == 0 && sentiment_idx < sentiment_flow.size() - 1) {
            SentimentType next_sentiment = sentiment_flow[(sentiment_idx + 1) % sentiment_flow.size()];
            double position = static_cast<double>(byte_idx) / encrypted.size();
            
            std::string transition_prompt = SentimentPromptBuilder::build_transition_prompt(
                current_sentiment, next_sentiment, position);
            
            auto& transition_gen = generators_[current_sentiment];
            output << "\n\n" << transition_gen->generate_ollama(transition_prompt, 100) << "\n\n";
        }
    }
    
    // Conclusion
    auto& conclusion_gen = generators_[cover_sentiment];
    output << "\n\n" << conclusion_gen->generate_ollama(
        "In conclusion", 150 + rand() % 100);
    
    return output.str();
}

/**
 * @brief Generate pure honeypot
 */
std::string OllamaCognitiveStegano::generate_pure_honeypot(
    size_t length,
    const std::vector<SentimentType>& sentiment_flow) {
    
    std::stringstream output;
    size_t current_length = 0;
    size_t sentiment_idx = 0;
    
    while (current_length < length) {
        SentimentType sentiment = sentiment_flow[sentiment_idx % sentiment_flow.size()];
        auto& generator = generators_[sentiment];
        
        // Mix honeypot and regular text
        if (rand() % 2 == 0) {
            output << generator->generate_honeypot(true) << " ";
        } else {
            output << generator->generate_ollama("", 100 + rand() % 100) << " ";
        }
        
        current_length = output.str().length();
        sentiment_idx++;
        
        // Add occasional cryptographic red herrings
        if (rand() % 10 == 0) {
            output << "[0x" << std::hex << (rand() % 0xFFFF) << std::dec << "] ";
        }
    }
    
    return output.str();
}

/**
 * @brief Analyze coherence using LLM
 */
double OllamaCognitiveStegano::analyze_coherence(const std::string& text) {
    auto& analyzer = generators_[SentimentType::PHILOSOPHICAL];
    
    std::stringstream prompt;
    prompt << "Rate the coherence of the following text on a scale of 0-100, "
           << "where 0 is completely incoherent and 100 is perfectly coherent. "
           << "Only respond with a number:\n\n" << text.substr(0, 500);
    
    std::string response = static_cast<OllamaSentimentGenerator*>(analyzer.get())->generate_ollama(prompt.str(), 100, 0.5);
    
    // Extract number from response
    double score = 50.0; // default
    try {
        size_t pos = response.find_first_of("0123456789");
        if (pos != std::string::npos) {
            score = std::stod(response.substr(pos));
        }
    } catch (...) {
        // Use default
    }
    
    return score / 100.0;
}

} // namespace goldenhash