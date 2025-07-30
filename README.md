# GoldenHash

A high-performance hash function based on the golden ratio and prime number theory.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## Overview

GoldenHash is a novel hash function that leverages the mathematical properties of the golden ratio (φ = 1.618...) to achieve exceptional distribution properties and performance. By selecting prime multipliers near N/φ and N/φ² (where N is the hash table size), GoldenHash provides:

- **Excellent Distribution**: Chi-square mean of 1.0002 (ideal: 1.0) across 5,000+ table sizes
- **Perfect Avalanche Effect**: 99.9% of table sizes achieve avalanche scores within the ideal 0.45-0.55 range
- **O(1) Performance**: Consistent 25-35 ns/hash regardless of table size
- **Minimal Collisions**: Collision rates match birthday paradox predictions

## Key Features

- **Dual Prime Architecture**: Uses two carefully selected primes for optimal mixing
- **Table-Size Awareness**: Incorporates table size into the chaos factor for size-dependent behavior
- **Single Modulo Operation**: Only one modulo at the end for maximum performance
- **Position-Dependent Mixing**: Ensures order sensitivity in input data
- **Comprehensive Testing**: Validated across 5,000 diverse table sizes from 255 to 10M+

## Quick Start

### Building from Source

```bash
# Clone the repository
git clone https://github.com/joshmorgan1000/goldenhash.git
cd goldenhash

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j

# Run tests
./goldenhash_test 1024 100000
```

### Basic Usage

```cpp
#include <goldenhash.hpp>

using namespace goldenhash;

// Create a hash function for table size 1024
GoldenHash hasher(1024);

// Hash some data
const char* data = "Hello, World!";
uint64_t hash = hasher.hash(
    reinterpret_cast<const uint8_t*>(data), 
    strlen(data)
);

// Get hash table index
std::cout << "Hash: " << hash << std::endl;
```

### Advanced Usage with Custom Seed

```cpp
// Create hasher with custom seed for different hash sequences
GoldenHash hasher(1024, 0xDEADBEEF);

// Hash variable-length data
std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
uint64_t hash = hasher.hash(data.data(), data.size());
```

## Performance

GoldenHash achieves consistent O(1) performance across all table sizes:

| Table Size | Time per Hash | Avalanche Score | Chi-Square |
|------------|---------------|-----------------|------------|
| 1,024      | 28.3 ns       | 0.492          | 0.998      |
| 65,536     | 29.1 ns       | 0.491          | 1.001      |
| 1,048,576  | 30.2 ns       | 0.493          | 0.999      |
| 10,000,000 | 31.5 ns       | 0.490          | 1.002      |

## Documentation

- [Algorithm Details](docs/golden_prime_hash.md) - Comprehensive explanation of the algorithm
- [Research Paper](docs/golden_prime_hash.pdf) - Full academic paper with proofs and analysis
- [Performance Analysis](docs/goldenhash_analysis.png) - Visual performance metrics

## Building the Documentation

The project includes tools for comprehensive performance analysis:

```bash
# Generate whitepaper results (requires Python)
cd python
source ~/venv-metal/bin/activate  # Or your Python environment
python generate_whitepaper_results.py

# Analyze results and create visualizations
python analyze_modular_results.py

# Build LaTeX documentation (requires LaTeX)
cd ../docs
pdflatex golden_prime_hash.tex
pdflatex golden_prime_hash.tex  # Run twice for references
```

## Implementation Details

### Key Optimizations

1. **Prime Caching**: Nearest prime calculations performed once during initialization
2. **Secret Array Precomputation**: 24-element mixing array computed once and reused
3. **Minimal Branching**: Main hash loop contains no conditional branches
4. **Single Modulo**: Only one modulo operation at the very end
5. **128-bit Multiplication**: Uses compiler intrinsics for efficient wide multiplication

### Memory Footprint

The GoldenHash class has a compact memory footprint of approximately 300 bytes:

```cpp
class GoldenHash {
    uint64_t N;               // 8 bytes - table size
    uint64_t prime_high;      // 8 bytes - prime near N/φ
    uint64_t prime_low;       // 8 bytes - prime near N/φ²
    uint64_t working_mod;     // 8 bytes - modulus for operations
    uint64_t seed_;           // 8 bytes - seed value
    vector<uint64_t> secret;  // 24 * 8 = 192 bytes
    vector<uint64_t> factors; // Variable, typically < 64 bytes
};
```

## Table Size Selection Guidelines

For optimal performance:

1. **Prefer Prime Numbers**: Prime table sizes provide the best avalanche properties
2. **Avoid Powers of 2**: Table sizes with many factors of 2 may show degraded avalanche
3. **Use Near-Primes**: If prime sizes are impractical, use numbers with few small factors
4. **Multiple Tables**: For cryptographic applications, use multiple coprime table sizes

## Requirements

- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- CMake 3.15 or higher
- Python 3.8+ (for analysis tools)
- Optional: LaTeX for building documentation

## Testing

The project includes comprehensive testing utilities:

```bash
# Basic correctness test
./goldenhash_test 1024 100000

# JSON output for analysis
./goldenhash_test 1024 100000 --json

# Run full test suite (5000+ table sizes)
cd python
python generate_whitepaper_results.py
```

## Contributing

Contributions are welcome! Please read our [Contributing Guidelines](CONTRIBUTING.md) before submitting PRs.

Areas of interest:
- Formal mathematical proofs
- SIMD optimizations
- Language bindings (Rust, Go, Python)
- Integration with existing hash table implementations

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Author

**Josh Morgan** - Initial work and research

## Acknowledgments

- Inspired by the mathematical properties of the golden ratio
- Performance testing framework adapted from xxHash benchmarks
- Statistical analysis based on SMHasher test suite

## Citation

If you use GoldenHash in your research, please cite:

```bibtex
@article{morgan2025goldenhash,
  title={GoldenHash: A High-Performance Hash Function Based on the Golden Ratio and Prime Number Theory},
  author={Morgan, Josh},
  year={2025},
  month={January}
}
```

## See Also

- [xxHash](https://github.com/Cyan4973/xxHash) - Extremely fast hash algorithm
- [CityHash](https://github.com/google/cityhash) - Google's family of hash functions
- [SMHasher](https://github.com/aappleby/smhasher) - Hash function quality testing
