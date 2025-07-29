#!/usr/bin/env python3
"""
Run CROCS tests for a specific configuration file
Records results directly to SQLite database
"""

import json
import sys
import subprocess
import time
import os
from data_collector import CROCSDataCollector

def ensure_test_built():
    """Ensure test executable exists"""
    if not os.path.exists("build/test_single_size"):
        print("Building test executable...")
        result = subprocess.run(
            ["cmake", "--build", "build", "--target", "test_single_size"],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print(f"Build failed: {result.stderr}")
            return False
    return True

def run_single_test(size, test_count=100000):
    """Run test for a single table size"""
    cmd = [
        "./test_single_size",
        f"--size={size}",
        f"--tests={test_count}",
        "--csv-output"
    ]
    
    try:
        result = subprocess.run(
            cmd,
            cwd="build",
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )
        
        if result.returncode != 0:
            print(f"  ERROR for size {size}: {result.stderr}")
            return None
            
        # Parse CSV output
        lines = result.stdout.strip().split('\n')
        if len(lines) < 2:
            return None
            
        # Find the data line (skip header)
        for line in lines:
            if line and not line.startswith("table_size"):
                parts = line.split(',')
                if len(parts) >= 12:
                    return {
                        'table_size': int(parts[0]),
                        'golden_prime': int(parts[1]),
                        'chi_square': float(parts[2]),
                        'collision_ratio': float(parts[3]),
                        'avalanche_score': float(parts[4]),
                        'distribution_uniformity': float(parts[5]),
                        'ns_per_hash': float(parts[6]),
                        'unique_hashes': int(parts[7]),
                        'total_collisions': int(parts[8]),
                        'max_bucket_load': int(parts[9]),
                        'bits_needed': int(parts[10]),
                        'prime_distance': int(parts[11])
                    }
        
        return None
        
    except subprocess.TimeoutExpired:
        print(f"  TIMEOUT for size {size}")
        return None
    except Exception as e:
        print(f"  EXCEPTION for size {size}: {e}")
        return None

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 run_config_tests.py config_NAME.json")
        sys.exit(1)
    
    config_file = sys.argv[1]
    
    # Load configuration
    try:
        with open(config_file, 'r') as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Configuration file '{config_file}' not found")
        sys.exit(1)
    
    print(f"Running configuration: {config['name']}")
    print(f"Description: {config['description']}")
    print(f"Number of sizes: {len(config['sizes'])}")
    print(f"Tests per size: {config['test_count']:,}")
    
    # Ensure test is built
    if not ensure_test_built():
        print("Failed to build test executable")
        sys.exit(1)
    
    # Initialize data collector
    collector = CROCSDataCollector()
    
    # Create test run
    run_id = collector.create_test_run(
        test_type=f"config_{config['name']}",
        parameters={
            "config_file": config_file,
            "num_sizes": len(config['sizes']),
            "test_count": config['test_count']
        },
        notes=config['description']
    )
    
    print(f"Created test run ID: {run_id}")
    
    # Run tests
    start_time = time.time()
    successful = 0
    failed = 0
    
    for i, size in enumerate(config['sizes']):
        print(f"\n[{i+1}/{len(config['sizes'])}] Testing size {size}...", end='', flush=True)
        
        test_start = time.time()
        result = run_single_test(size, config['test_count'])
        test_time = time.time() - test_start
        
        if result:
            # Store in database
            cursor = collector.conn.cursor()
            cursor.execute("""
            INSERT INTO hash_tests (
                run_id, table_size, golden_prime, chi_square, collision_ratio,
                avalanche_score, distribution_uniformity, ns_per_hash,
                unique_hashes, total_collisions, max_bucket_load,
                bits_needed, prime_distance
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                run_id,
                result['table_size'],
                result['golden_prime'],
                result['chi_square'],
                result['collision_ratio'],
                result['avalanche_score'],
                result['distribution_uniformity'],
                result['ns_per_hash'],
                result['unique_hashes'],
                result['total_collisions'],
                result['max_bucket_load'],
                result['bits_needed'],
                result['prime_distance']
            ))
            collector.conn.commit()
            
            print(f" ✓ ({test_time:.1f}s)")
            print(f"  Chi²: {result['chi_square']:.4f}, "
                  f"Avalanche: {result['avalanche_score']:.4f}, "
                  f"Performance: {result['ns_per_hash']:.2f} ns/hash")
            successful += 1
        else:
            print(f" ✗ ({test_time:.1f}s)")
            failed += 1
        
        # Progress estimate
        elapsed = time.time() - start_time
        if i > 0:
            avg_time = elapsed / (i + 1)
            remaining = avg_time * (len(config['sizes']) - i - 1)
            print(f"  Progress: {100*(i+1)/len(config['sizes']):.1f}% "
                  f"(~{remaining/60:.1f} min remaining)")
    
    # Summary
    total_time = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"Configuration '{config['name']}' complete!")
    print(f"{'='*60}")
    print(f"Total time: {total_time/60:.1f} minutes")
    print(f"Successful tests: {successful}")
    print(f"Failed tests: {failed}")
    print(f"Average time per test: {total_time/len(config['sizes']):.2f} seconds")
    
    # Quick analysis for this run
    cursor = collector.conn.cursor()
    cursor.execute("""
    SELECT 
        AVG(chi_square) as avg_chi,
        AVG(avalanche_score) as avg_avalanche,
        AVG(ns_per_hash) as avg_perf,
        MIN(avalanche_score) as min_avalanche,
        MAX(avalanche_score) as max_avalanche
    FROM hash_tests
    WHERE run_id = ?
    """, (run_id,))
    
    stats = cursor.fetchone()
    if stats[0]:  # If we have results
        print(f"\nRun statistics:")
        print(f"  Average chi-square: {stats[0]:.4f}")
        print(f"  Average avalanche: {stats[1]:.4f}")
        print(f"  Avalanche range: {stats[3]:.4f} - {stats[4]:.4f}")
        print(f"  Average performance: {stats[2]:.2f} ns/hash")
    
    collector.close()

if __name__ == "__main__":
    main()