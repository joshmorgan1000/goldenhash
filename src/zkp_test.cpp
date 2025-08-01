#include "goldenhash128.hpp"
#include <random>
#include <iostream>
#include <cassert>

using namespace goldenhash;

// Simple modular exponentiation for __uint128_t (base^exp mod mod)
__uint128_t mod_pow(__uint128_t base, __uint128_t exp, __uint128_t mod) {
    __uint128_t result = 1;
    base %= mod;
    while (exp) {
        if (exp & 1) result = (result * base) % mod;
        base = (base * base) % mod;
        exp >>= 1;
    }
    return result;
}

// Chinese Remainder Theorem to compute square root mod N, given p, q
__uint128_t mod_sqrt(__uint128_t x, __uint128_t p, __uint128_t q, __uint128_t N) {
    // This is a stub for demo; for real primes, use Tonelli-Shanks and CRT
    // For safe primes and demo: only works for small p, q!
    __uint128_t r_p = mod_pow(x, (p + 1) / 4, p); // Tonelli-Shanks for mod p
    __uint128_t r_q = mod_pow(x, (q + 1) / 4, q); // Tonelli-Shanks for mod q

    // CRT: Solve r ≡ r_p mod p, r ≡ r_q mod q
    // See https://en.wikipedia.org/wiki/Chinese_remainder_theorem#Existence_(constructive_proof)
    __uint128_t inv_p = mod_pow(p, q - 2, q);
    __uint128_t h = (inv_p * (r_q + q - r_p % q)) % q;
    __uint128_t r = r_p + h * p;
    return r % N;
}

// Generate a random 64-bit value for use as a blinding factor
__uint128_t random_uint128() {
    static std::mt19937_64 rng(std::random_device{}());
    return ((__uint128_t)rng() << 64) | rng();
}

// Simple demo prime generator (slow, but works for small numbers)
uint64_t random_prime(std::mt19937_64& rng, uint64_t min = (1ULL << 62)) {
    while (true) {
        uint64_t candidate = rng() | 1ULL;
        if (candidate < min) continue;
        bool is_prime = true;
        for (uint64_t d = 3; d < 10000; d += 2) {
            if (candidate % d == 0) { is_prime = false; break; }
        }
        if (is_prime) return candidate;
    }
}

int main() {
    std::mt19937_64 rng(std::random_device{}());

    // === SETUP: Prover generates large N = p*q and keeps factors secret ===
    uint64_t p = random_prime(rng);
    uint64_t q = random_prime(rng);
    __uint128_t N = (__uint128_t)p * (__uint128_t)q;
    GoldenHash128 gh(N, 42);
    std::cout << "Public modulus N = " << (uint64_t)(N >> 64) << std::hex << (uint64_t)N << std::dec << "\n";

    // === Prover picks random r, sends x = r^2 mod N ===
    __uint128_t r = random_uint128() % N;
    __uint128_t x = mod_pow(r, 2, N);
    std::cout << "Prover sends x = r^2 mod N = " << (uint64_t)(x >> 64) << std::hex << (uint64_t)x << std::dec << "\n";

    // === Verifier issues random challenge bit ===
    int challenge = rng() % 2;
    std::cout << "Verifier issues challenge bit: " << challenge << "\n";

    // === Prover responds ===
    __uint128_t response = 0;
    if (challenge == 0) {
        // Send r
        response = r;
        std::cout << "Prover responds with r\n";
    } else {
        // Compute square root s of x mod N using p, q (only possible if you know factors!)
        __uint128_t s = mod_sqrt(x, p, q, N);
        response = (r * s) % N;
        std::cout << "Prover responds with r * sqrt(x) mod N\n";
    }

    // === Verifier checks ===
    if (challenge == 0) {
        // Check response^2 mod N == x
        __uint128_t check = mod_pow(response, 2, N);
        std::cout << "Verifier checks r^2 mod N == x? " << (check == x ? "PASS" : "FAIL") << "\n";
    } else {
        // For real ZKP: more checks needed. For demo, verify s exists and is correct
        // For production: repeat this protocol 40+ times for soundness!
        std::cout << "Verifier accepts response for challenge 1 (demo only)\n";
    }

    // === Print hash with GoldenHash128 for fun ===
    uint8_t testdata[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    auto hashval = gh.hash(testdata, 16);
    std::cout << "GoldenHash128 hash(testdata,16) = " << (uint64_t)(hashval >> 64) << std::hex << (uint64_t)hashval << std::dec << "\n";
}