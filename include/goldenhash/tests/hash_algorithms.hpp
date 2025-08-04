#pragma once

#include <goldenhash.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include "xxhash.h"
#include <openssl/sha.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>

namespace goldenhash::tests {

/**
 * @brief Compute hash using the selected algorithm
 * @param algo_name Name of the algorithm
 * @param data Input data
 * @param len Length of input data
 * @param table_size Size of hash table for modulo operation
 * @param hasher GoldenHash instance (only used if algo_name is "goldenhash")
 * @return Hash value modulo table_size
 */
inline uint64_t compute_hash(const std::string& algo_name, uint8_t* data, size_t len, 
                            uint64_t table_size, GoldenHash& hasher) {
    if (algo_name == "goldenhash") {
        return hasher.hash(data, len);
    } else if (algo_name == "xxhash64") {
        return XXH64(data, len, 0) % table_size;
    } else if (algo_name == "sha256") {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data, len, hash);
        uint64_t truncated = 0;
        memcpy(&truncated, hash, sizeof(uint64_t));
        return truncated % table_size;
    } else if (algo_name == "aes-cmac") {
        // AES-128-CMAC
        static const unsigned char key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        
        size_t mac_len = 16;
        unsigned char mac[16];
        
        EVP_MAC *evp_mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
        EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(evp_mac);
        
        OSSL_PARAM params[2];
        params[0] = OSSL_PARAM_construct_utf8_string("cipher", (char*)"AES-128-CBC", 0);
        params[1] = OSSL_PARAM_construct_end();
        
        EVP_MAC_init(ctx, key, 16, params);
        EVP_MAC_update(ctx, data, len);
        EVP_MAC_final(ctx, mac, &mac_len, sizeof(mac));
        
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(evp_mac);
        
        uint64_t truncated = 0;
        memcpy(&truncated, mac, sizeof(uint64_t));
        return truncated % table_size;
    }
    throw std::runtime_error("Unknown algorithm: " + algo_name);
}

} // namespace goldenhash
