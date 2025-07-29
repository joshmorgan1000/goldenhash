#!/usr/bin/env python3
"""
Massive CROCS test runner - generates thousands of test cases
Designed to run for hours and collect comprehensive data
"""

import subprocess
import time
import random
from data_collector import CROCSDataCollector

# Generate test sizes
def generate_all_test_sizes():
    sizes = []
    
    # Every prime from 53 to 10,000
    def is_prime(n):
        if n < 2: return False
        for i in range(2, int(n**0.5) + 1):
            if n % i == 0: return False
        return True
    
    for n in range(53, 10000):
        if is_prime(n):
            sizes.append(n)
    
    # Powers of 2 and nearby
    for p in range(6, 24):
        base = 2**p
        sizes.extend([base-1, base, base+1])
    
    # Powers of 10 and nearby
    for p in range(2, 8):
        base = 10**p
        sizes.extend([base-1, base, base+1])
    
    # Random sizes
    for _ in range(1000):
        sizes.append(random.randint(100, 1000000))
    
    # Remove duplicates and sort
    return sorted(list(set(sizes)))

def main():
    print("CROCS Massive Test Suite")
    print("=" * 60)
    
    sizes = generate_all_test_sizes()
    print(f"Generated {len(sizes)} test sizes")
    
    # Test parameters
    TEST_COUNTS = {
        "small": 100000,    # < 1K
        "medium": 500000,   # 1K - 100K  
        "large": 1000000,   # > 100K
    }
    
    collector = CROCSDataCollector()
    run_id = collector.create_test_run("massive_suite", 
                                      {"total_sizes": len(sizes)},
                                      "Comprehensive test of all sizes")
    
    print(f"Starting at {time.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"Estimated time: {len(sizes) * 30 / 3600:.1f} hours")
    print("=" * 60)
    
    start_time = time.time()
    
    for i, size in enumerate(sizes):
        # Determine test count based on size
        if size < 1000:
            test_count = TEST_COUNTS["small"]
        elif size < 100000:
            test_count = TEST_COUNTS["medium"]
        else:
            test_count = TEST_COUNTS["large"]
        
        print(f"\n[{i+1}/{len(sizes)}] Size {size} with {test_count:,} tests...", end='', flush=True)
        
        # Run test
        cmd = ["./build/test_comprehensive", f"--size={size}", f"--tests={test_count}", "--csv-output"]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            
            if result.returncode == 0 and result.stdout:
                # Parse CSV line
                lines = result.stdout.strip().split('\n')
                for line in lines:
                    if line and not line.startswith("table_size"):
                        parts = line.split(',')
                        if len(parts) >= 12:
                            # Store in database
                            cursor = collector.conn.cursor()
                            cursor.execute("""
                            INSERT INTO hash_tests (
                                run_id, table_size, golden_prime, chi_square, collision_ratio,
                                avalanche_score, distribution_uniformity, ns_per_hash,
                                unique_hashes, total_collisions, max_bucket_load,
                                bits_needed, prime_distance
                            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                            """, (run_id, *[float(p) if '.' in p else int(p) for p in parts[:12]]))
                            collector.conn.commit()
                            
                            chi = float(parts[2])
                            avalanche = float(parts[4])
                            print(f" ✓ (χ²={chi:.3f}, avalanche={avalanche:.3f})")
                            break
            else:
                print(" ✗")
                
        except subprocess.TimeoutExpired:
            print(" TIMEOUT")
        except Exception as e:
            print(f" ERROR: {e}")
        
        # Progress estimate
        elapsed = time.time() - start_time
        avg_time = elapsed / (i + 1)
        remaining = avg_time * (len(sizes) - i - 1)
        print(f"Progress: {100*(i+1)/len(sizes):.1f}%, ETA: {remaining/3600:.1f} hours")
    
    # Final summary
    total_time = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"Completed {len(sizes)} tests in {total_time/3600:.1f} hours")
    print(f"Average time per test: {total_time/len(sizes):.1f} seconds")
    
    collector.close()

if __name__ == "__main__":
    main()