#include <cfenv>
#include <cmath>
#include <cstdint>

namespace goldenhash {

double golden_ratio = 1.6180339887498948482;

uint64_t GoldenHash::float_hash(uint64_t key, uint64_t seed, uint64_t table_size) {
    // Ensure deterministic rounding
    std::fesetround(FE_TONEAREST);

    double x = static_cast<double>(key ^ seed);
    double mix = x * golden_ratio * static_cast<double>(table_size);
    
    // Map into [0, 1) range deterministically
    double frac = mix - std::floor(mix);
    
    // Scale to integer space (e.g., 64-bit)
    return static_cast<uint64_t>(frac * 0xFFFFFFFFFFFFFFFFull);
}

}
