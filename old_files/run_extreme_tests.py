#!/usr/bin/env python3
"""
Extreme CROCS testing - millions of tests per configuration
"""

import subprocess
import time
import math
from data_collector import CROCSDataCollector

# Test configurations with appropriate test counts
EXTREME_CONFIGS = [
    # Small tables (high test counts)
    {"sizes": range(97, 1000, 50), "tests": 1000000},
    {"sizes": range(1009, 10000, 500), "tests": 10000000},
    
    # Medium tables
    {"sizes": range(10007, 100000, 5000), "tests": 50000000},
    
    # Large tables  
    {"sizes": range(100003, 1000000, 50000), "tests": 100000000},
    
    # Huge tables
    {"sizes": [1000003, 2000003, 4000037, 8000009, 16000057], "tests": 200000000},
]

def calculate_appropriate_tests(table_size):
    """Calculate appropriate number of tests based on table size"""
    # Want to test at least 100x the table size for good statistics
    # But cap based on time constraints
    base = max(table_size * 100, 1000000)
    
    # Scale up for larger tables
    if table_size > 100000:
        base *= 10
    if table_size > 1000000:
        base *= 10
        
    # Cap at 500M for time reasons
    return min(base, 500000000)

def main():
    print("CROCS EXTREME Testing Suite")
    print("=" * 60)
    print("WARNING: This will take MANY hours!")
    print("=" * 60)
    
    collector = CROCSDataCollector()
    run_id = collector.create_test_run("extreme_suite", 
                                      {"config": "extreme"},
                                      "Extreme testing with millions of operations")
    
    total_sizes = sum(len(list(config["sizes"])) for config in EXTREME_CONFIGS)
    print(f"Total configurations: {total_sizes}")
    
    # Estimate time
    total_est_time = 0
    for config in EXTREME_CONFIGS:
        for size in config["sizes"]:
            # ~80ns per hash + overhead
            est_seconds = (config["tests"] * 100e-9) + 5  # 100ns estimate + 5s overhead
            total_est_time += est_seconds
    
    print(f"Estimated time: {total_est_time/3600:.1f} hours")
    print()
    
    completed = 0
    start_time = time.time()
    
    for config in EXTREME_CONFIGS:
        for size in config["sizes"]:
            completed += 1
            tests = calculate_appropriate_tests(size)
            
            print(f"[{completed}/{total_sizes}] Size {size} with {tests/1e6:.1f}M tests... ", 
                  end='', flush=True)
            
            test_start = time.time()
            
            # Run with environment variable to skip avalanche for speed
            cmd = ["./build/test_comprehensive", 
                   f"--size={size}", 
                   f"--tests={tests}", 
                   "--csv-output"]
            
            env = {"SKIP_AVALANCHE": "1"}  # Skip avalanche for extreme tests
            
            try:
                result = subprocess.run(cmd, 
                                      capture_output=True, 
                                      text=True,
                                      timeout=3600,  # 1 hour timeout
                                      env=env)
                
                if result.returncode == 0 and result.stdout:
                    # Parse and store result
                    lines = result.stdout.strip().split('\n')
                    for line in lines:
                        if line and not line.startswith("table_size"):
                            parts = line.split(',')
                            if len(parts) >= 12:
                                cursor = collector.conn.cursor()
                                cursor.execute("""
                                INSERT INTO hash_tests (
                                    run_id, table_size, golden_prime, chi_square, 
                                    collision_ratio, avalanche_score, distribution_uniformity, 
                                    ns_per_hash, unique_hashes, total_collisions, 
                                    max_bucket_load, bits_needed, prime_distance
                                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                                """, (run_id, *[float(p) if '.' in p else int(p) for p in parts[:12]]))
                                collector.conn.commit()
                                
                                test_time = time.time() - test_start
                                chi = float(parts[2])
                                print(f"✓ ({test_time:.1f}s, χ²={chi:.4f})")
                                break
                else:
                    print("✗")
                    
            except subprocess.TimeoutExpired:
                print("TIMEOUT")
            except Exception as e:
                print(f"ERROR: {e}")
            
            # Progress
            elapsed = time.time() - start_time
            remaining = (elapsed / completed) * (total_sizes - completed)
            print(f"  Progress: {100*completed/total_sizes:.1f}%, ETA: {remaining/3600:.1f}h")
    
    # Summary
    total_time = time.time() - start_time
    
    # Get statistics
    cursor = collector.conn.cursor()
    cursor.execute("""
    SELECT 
        COUNT(*) as count,
        AVG(chi_square) as avg_chi,
        MIN(chi_square) as min_chi,
        MAX(chi_square) as max_chi,
        AVG(ns_per_hash) as avg_perf
    FROM hash_tests
    WHERE run_id = ?
    """, (run_id,))
    
    stats = cursor.fetchone()
    
    print(f"\n{'='*60}")
    print("EXTREME Testing Complete!")
    print(f"{'='*60}")
    print(f"Total time: {total_time/3600:.1f} hours")
    print(f"Tests completed: {stats[0]}")
    print(f"Chi-square: {stats[1]:.4f} (min: {stats[2]:.4f}, max: {stats[3]:.4f})")
    print(f"Avg performance: {stats[4]:.2f} ns/hash")
    
    collector.close()

if __name__ == "__main__":
    # Sanity check
    response = input("This will run for HOURS. Continue? (yes/no): ")
    if response.lower() != "yes":
        print("Aborted.")
    else:
        main()