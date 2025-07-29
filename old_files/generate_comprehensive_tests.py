#!/usr/bin/env python3
"""
Generate comprehensive CROCS test configurations
Creates thousands of test cases across different categories
"""

import json
import math

def is_prime(n):
    """Simple primality test"""
    if n < 2:
        return False
    if n == 2:
        return True
    if n % 2 == 0:
        return False
    for i in range(3, int(math.sqrt(n)) + 1, 2):
        if n % i == 0:
            return False
    return True

def next_prime(n):
    """Find next prime >= n"""
    if n <= 2:
        return 2
    if n % 2 == 0:
        n += 1
    while not is_prime(n):
        n += 2
    return n

def generate_test_sizes():
    """Generate comprehensive list of test sizes"""
    sizes = []
    
    # 1. Small primes (every prime up to 1000)
    for n in range(2, 1000):
        if is_prime(n):
            sizes.append(n)
    
    # 2. Powers of 2 and nearby values
    for p in range(6, 24):  # 64 to 8M
        base = 2**p
        sizes.extend([
            base - 1,
            base,
            base + 1,
            next_prime(base - 10),
            next_prime(base + 10)
        ])
    
    # 3. Powers of 10 and nearby primes
    for p in range(2, 8):  # 100 to 10M
        base = 10**p
        sizes.extend([
            base,
            next_prime(base - 100),
            next_prime(base),
            next_prime(base + 100)
        ])
    
    # 4. Fibonacci numbers and nearby primes
    a, b = 1, 1
    while b < 10_000_000:
        sizes.append(b)
        sizes.append(next_prime(b))
        a, b = b, a + b
    
    # 5. Mersenne primes and related
    for p in [2, 3, 5, 7, 13, 17, 19, 31]:
        mersenne = 2**p - 1
        if mersenne < 10_000_000:
            sizes.extend([
                mersenne,
                2**p,
                2**p + 1
            ])
    
    # 6. Common hash table sizes
    common_sizes = [
        97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
        49157, 98317, 196613, 393241, 786433, 1572869,
        3145739, 6291469
    ]
    sizes.extend(common_sizes)
    
    # 7. "Bad" sizes (highly composite numbers)
    bad_sizes = [
        120, 360, 720, 1260, 1680, 2520, 5040, 10080,
        20160, 40320, 80640, 181440, 362880, 725760
    ]
    sizes.extend(bad_sizes)
    
    # 8. Random-looking primes at different scales
    random_primes = [
        127, 251, 509, 1021, 2039, 4093, 8191, 16381,
        32749, 65521, 131071, 262139, 524287, 1048573,
        2097143, 4194301, 8388593
    ]
    sizes.extend(random_primes)
    
    # 9. Golden ratio related sizes
    phi = 1.6180339887498948482
    for i in range(10, 25):
        golden = int(2**i / phi)
        sizes.extend([
            golden - 1,
            golden,
            golden + 1,
            next_prime(golden)
        ])
    
    # 10. Square numbers and nearby
    for i in range(10, 1000, 10):
        sizes.extend([
            i * i - 1,
            i * i,
            i * i + 1
        ])
    
    # Remove duplicates and sort
    sizes = sorted(list(set(sizes)))
    
    # Filter to reasonable range
    sizes = [s for s in sizes if 50 <= s <= 10_000_000]
    
    return sizes

def categorize_sizes(sizes):
    """Categorize sizes for different test configurations"""
    categories = {
        "tiny": [],        # 50-500
        "small": [],       # 500-5K
        "medium": [],      # 5K-50K
        "large": [],       # 50K-500K
        "huge": [],        # 500K-5M
        "gigantic": [],    # 5M+
        "powers_of_2": [],
        "mersenne": [],
        "fibonacci": [],
        "highly_composite": []
    }
    
    # Fibonacci numbers for detection
    fibs = set()
    a, b = 1, 1
    while b < 10_000_000:
        fibs.add(b)
        a, b = b, a + b
    
    for s in sizes:
        # Size categories
        if s < 500:
            categories["tiny"].append(s)
        elif s < 5000:
            categories["small"].append(s)
        elif s < 50000:
            categories["medium"].append(s)
        elif s < 500000:
            categories["large"].append(s)
        elif s < 5000000:
            categories["huge"].append(s)
        else:
            categories["gigantic"].append(s)
        
        # Special categories
        if s & (s - 1) == 0:  # Power of 2
            categories["powers_of_2"].append(s)
        
        # Check if Mersenne (2^p - 1)
        if s + 1 in categories["powers_of_2"]:
            categories["mersenne"].append(s)
        
        if s in fibs:
            categories["fibonacci"].append(s)
        
        # Highly composite if has many factors
        factor_count = sum(1 for i in range(1, int(s**0.5) + 1) if s % i == 0)
        if factor_count > s**0.25:  # Heuristic for "many factors"
            categories["highly_composite"].append(s)
    
    return categories

def create_test_configs(categories):
    """Create test configurations for different scenarios"""
    configs = []
    
    # 1. Quick smoke test (select representatives)
    quick_sizes = []
    for cat in ["tiny", "small", "medium", "large"]:
        if categories[cat]:
            # Take first, middle, and last from each category
            cat_sizes = categories[cat]
            quick_sizes.append(cat_sizes[0])
            if len(cat_sizes) > 2:
                quick_sizes.append(cat_sizes[len(cat_sizes)//2])
            quick_sizes.append(cat_sizes[-1])
    
    configs.append({
        "name": "quick_smoke",
        "description": "Quick smoke test with representative sizes",
        "sizes": sorted(list(set(quick_sizes))),
        "test_count": 10000
    })
    
    # 2. Comprehensive small (all tiny + small)
    configs.append({
        "name": "comprehensive_small",
        "description": "All sizes under 5K",
        "sizes": sorted(categories["tiny"] + categories["small"]),
        "test_count": 100000
    })
    
    # 3. Powers of 2 focus
    configs.append({
        "name": "powers_of_2",
        "description": "Powers of 2 and nearby values",
        "sizes": sorted(categories["powers_of_2"]),
        "test_count": 100000
    })
    
    # 4. Mathematical sequences
    math_sizes = sorted(list(set(
        categories["mersenne"] + 
        categories["fibonacci"] + 
        categories["powers_of_2"]
    )))
    configs.append({
        "name": "mathematical",
        "description": "Mersenne, Fibonacci, and powers of 2",
        "sizes": math_sizes,
        "test_count": 100000
    })
    
    # 5. Stress test (highly composite)
    configs.append({
        "name": "stress_composite",
        "description": "Highly composite numbers (many factors)",
        "sizes": sorted(categories["highly_composite"])[:100],  # Limit to 100
        "test_count": 100000
    })
    
    # 6. Medium comprehensive
    configs.append({
        "name": "comprehensive_medium",
        "description": "All medium sizes (5K-50K)",
        "sizes": categories["medium"],
        "test_count": 100000
    })
    
    # 7. Large scale
    configs.append({
        "name": "large_scale",
        "description": "Large sizes (50K-500K)",
        "sizes": categories["large"][:200],  # Limit for performance
        "test_count": 1000000
    })
    
    # 8. Extreme scale
    configs.append({
        "name": "extreme_scale",
        "description": "Very large sizes (500K+)",
        "sizes": categories["huge"][:50] + categories["gigantic"][:10],
        "test_count": 1000000
    })
    
    # 9. Prime density study
    prime_ranges = []
    for base in [100, 1000, 10000, 100000, 1000000]:
        for i in range(base, base + 1000):
            if is_prime(i):
                prime_ranges.append(i)
    configs.append({
        "name": "prime_density",
        "description": "Prime density at different scales",
        "sizes": sorted(list(set(prime_ranges))),
        "test_count": 50000
    })
    
    # 10. Golden ratio focused
    golden_sizes = []
    phi = 1.6180339887498948482
    for n in range(100, 1000000, 1000):
        golden = int(n / phi)
        golden_sizes.extend([
            next_prime(golden - 10),
            next_prime(golden),
            next_prime(golden + 10)
        ])
    configs.append({
        "name": "golden_ratio_focus",
        "description": "Sizes related to golden ratio",
        "sizes": sorted(list(set(golden_sizes))),
        "test_count": 100000
    })
    
    return configs

def main():
    print("Generating comprehensive test configurations...")
    
    # Generate all test sizes
    all_sizes = generate_test_sizes()
    print(f"Generated {len(all_sizes)} unique test sizes")
    
    # Categorize them
    categories = categorize_sizes(all_sizes)
    
    # Print category summary
    print("\nCategory summary:")
    for cat, sizes in categories.items():
        if sizes:
            print(f"  {cat}: {len(sizes)} sizes")
    
    # Create test configurations
    configs = create_test_configs(categories)
    
    # Save configurations
    with open("test_configurations.json", "w") as f:
        json.dump({
            "total_unique_sizes": len(all_sizes),
            "configurations": configs
        }, f, indent=2)
    
    # Calculate total tests
    total_tests = sum(len(c["sizes"]) for c in configs)
    print(f"\nCreated {len(configs)} test configurations")
    print(f"Total test cases: {total_tests}")
    
    # Save individual config files
    for config in configs:
        filename = f"config_{config['name']}.json"
        with open(filename, "w") as f:
            json.dump(config, f, indent=2)
        print(f"  Saved {filename}: {len(config['sizes'])} sizes")
    
    # Create a master run script
    with open("run_all_tests.sh", "w") as f:
        f.write("#!/bin/bash\n\n")
        f.write("# Run all CROCS comprehensive tests\n")
        f.write("# This will take several hours\n\n")
        
        f.write("echo 'Starting comprehensive CROCS testing...'\n")
        f.write("echo 'Total configurations: {}'\n".format(len(configs)))
        f.write("echo 'Total test cases: {}'\n\n".format(total_tests))
        
        for config in configs:
            f.write(f"echo '\\n=== Running {config['name']} ==='\n")
            f.write(f"echo '{config['description']}'\n")
            f.write(f"echo 'Sizes: {len(config['sizes'])}'\n")
            f.write(f"python3 run_config_tests.py config_{config['name']}.json\n")
            f.write("sleep 2\n\n")
        
        f.write("echo '\\nAll tests complete!'\n")
        f.write("python3 data_collector.py analyze\n")
    
    print("\nCreated run_all_tests.sh")
    print("\nTo run all tests: bash run_all_tests.sh")
    print("To run specific config: python3 run_config_tests.py config_NAME.json")

if __name__ == "__main__":
    main()