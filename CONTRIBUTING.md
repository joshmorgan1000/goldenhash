# Contributing to GoldenHash

Thank you for your interest in contributing to GoldenHash! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

By participating in this project, you agree to abide by our principles of respectful and constructive collaboration.

## How to Contribute

### Reporting Issues

Before creating a new issue, please:
1. Check the existing issues to avoid duplicates
2. Use the issue search feature to see if the issue has already been reported
3. Include as much relevant information as possible:
   - Steps to reproduce the issue
   - Expected behavior
   - Actual behavior
   - System information (OS, compiler version, etc.)
   - Error messages or logs

### Suggesting Enhancements

Enhancement suggestions are welcome! Please:
1. Use a clear and descriptive title
2. Provide a detailed description of the proposed enhancement
3. Explain why this enhancement would be useful
4. Include examples or mockups if applicable

### Pull Requests

1. **Fork and Clone**
   ```bash
   git clone https://github.com/joshmorgan1000/goldenhash.git
   cd goldenhash
   git remote add upstream https://github.com/joshmorgan1000/goldenhash.git
   ```

2. **Create a Branch**
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bug-fix
   ```

3. **Make Your Changes**
   - Follow the existing code style
   - Add tests for new functionality
   - Update documentation as needed
   - Ensure all tests pass

4. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "Brief description of changes"
   ```
   
   Commit message guidelines:
   - Use present tense ("Add feature" not "Added feature")
   - Use imperative mood ("Move cursor to..." not "Moves cursor to...")
   - Limit first line to 72 characters
   - Reference issues and pull requests when relevant

5. **Push to Your Fork**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Submit a Pull Request**
   - Go to the original repository on GitHub
   - Click "New pull request"
   - Select your fork and branch
   - Fill in the PR template with all relevant information

## Development Setup

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- CMake 3.15 or higher
- Python 3.8+ (for testing tools)
- Git

### Building and Testing

```bash
# Build the project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j

# Run tests
./goldenhash_test 1024 100000

# Run performance analysis
cd ../python
python generate_whitepaper_results.py
python analyze_modular_results.py
```

## Code Style Guidelines

### C++ Style

- Use 4 spaces for indentation (no tabs)
- Opening braces on the same line for functions and control structures
- Use meaningful variable and function names
- Add Doxygen comments for all public APIs
- Follow const-correctness principles
- Prefer `uint64_t` over `unsigned long long`

Example:
```cpp
/**
 * @brief Calculate hash value for given data
 * @param data Pointer to input data
 * @param len Length of input data in bytes
 * @return Hash value in range [0, N)
 */
uint64_t hash(const uint8_t* data, size_t len) const {
    // Implementation
}
```

### Python Style

- Follow PEP 8
- Use type hints where appropriate
- Add docstrings to all functions and classes
- Use descriptive variable names

## Testing Guidelines

- Write tests for all new functionality
- Ensure tests cover edge cases
- Maintain or improve code coverage
- Test on multiple platforms if possible
- Performance tests should be reproducible

## Documentation

- Update README.md if adding new features or changing usage
- Update inline documentation for API changes
- Add examples for new functionality
- Keep the whitepaper in sync with implementation changes

## Areas of Interest

We're particularly interested in contributions in these areas:

### Performance Improvements
- SIMD optimizations for bulk hashing
- Platform-specific optimizations
- Cache-friendly implementations

### Mathematical Analysis
- Formal proofs of distribution properties
- Analysis of collision resistance
- Security properties investigation

### Language Bindings
- Rust FFI bindings
- Go wrapper
- Python C extension
- WebAssembly port

### Integration
- Hash table implementations using GoldenHash
- Database index integration
- Distributed systems applications

### Testing and Validation
- Additional statistical tests
- Benchmark comparisons with other hash functions
- Real-world performance testing

## Review Process

1. All submissions require review before merging
2. Reviewers will check for:
   - Code quality and style compliance
   - Test coverage
   - Documentation completeness
   - Performance impact
   - Compatibility

3. Address reviewer feedback promptly
4. Be patient - thorough reviews take time

## Recognition

Contributors will be recognized in:
- The project's contributors list
- Release notes for significant contributions
- Academic papers (for research contributions)

## Questions?

If you have questions about contributing, feel free to:
- Open a discussion issue
- Contact the maintainers
- Join our community discussions

Thank you for helping make GoldenHash better!