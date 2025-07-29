#!/usr/bin/env python3
"""
Systematic CROCS testing script
Runs comprehensive tests in background and populates database
"""

import subprocess
import time
import os
import sys
import json
from data_collector import CROCSDataCollector

# Test configurations
TEST_CONFIGS = {
    "small": {
        "table_sizes": [97, 191, 383, 769, 1543, 3079, 6151, 12289, 24593],
        "test_count": 10000,
        "description": "Small table sizes (< 25K)"
    },
    "medium": {
        "table_sizes": [49157, 98317, 196613, 393241, 786433, 1572869],
        "test_count": 100000,
        "description": "Medium table sizes (25K - 2M)"
    },
    "large": {
        "table_sizes": [3145739, 6291469, 12582917, 25165843, 50331653],
        "test_count": 1000000,
        "description": "Large table sizes (2M - 50M)"
    },
    "powers_of_2": {
        "table_sizes": [128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536],
        "test_count": 100000,
        "description": "Powers of 2"
    },
    "primes": {
        "table_sizes": [97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317],
        "test_count": 100000,
        "description": "Prime numbers"
    }
}

def build_test_executable():
    """Ensure test executable is built"""
    print("Building test executable...")
    result = subprocess.run(["cmake", "--build", ".", "--target", "test_crocs_comprehensive"], 
                          cwd="build", capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Build failed: {result.stderr}")
        return False
    print("Build successful")
    return True

def run_test_batch(collector, config_name, config):
    """Run a batch of tests with given configuration"""
    print(f"\n{'='*60}")
    print(f"Running {config_name}: {config['description']}")
    print(f"Table sizes: {config['table_sizes']}")
    print(f"Test count: {config['test_count']:,}")
    print(f"{'='*60}")
    
    # Create test run
    run_id = collector.create_test_run(
        test_type="systematic_batch",
        parameters={
            "config_name": config_name,
            "table_sizes": config['table_sizes'],
            "test_count": config['test_count']
        },
        notes=config['description']
    )
    
    # Run tests for each table size
    for table_size in config['table_sizes']:
        print(f"\nTesting table size {table_size:,}...")
        
        # Prepare command
        cmd = [
            "./test_crocs_comprehensive",
            "--table-size", str(table_size),
            "--test-count", str(config['test_count']),
            "--quiet"  # Suppress verbose output
        ]
        
        # Run test
        start_time = time.time()
        result = subprocess.run(cmd, cwd="build", capture_output=True, text=True)
        elapsed = time.time() - start_time
        
        if result.returncode != 0:
            print(f"  ERROR: {result.stderr}")
            continue
        
        # Parse output for key metrics
        lines = result.stdout.split('\n')
        metrics = {}
        
        for line in lines:
            if "Chi-square:" in line:
                metrics['chi_square'] = float(line.split(':')[1].strip())
            elif "Collision ratio:" in line:
                metrics['collision_ratio'] = float(line.split(':')[1].strip())
            elif "Performance:" in line:
                metrics['ns_per_hash'] = float(line.split(':')[1].split()[0])
            elif "Unique hashes:" in line:
                metrics['unique_hashes'] = int(line.split(':')[1].strip())
            elif "Total collisions:" in line:
                metrics['total_collisions'] = int(line.split(':')[1].strip())
            elif "Golden prime:" in line:
                metrics['golden_prime'] = int(line.split(':')[1].strip())
        
        # Store in database
        if metrics:
            cursor = collector.conn.cursor()
            cursor.execute("""
            INSERT INTO hash_tests (
                run_id, table_size, golden_prime, chi_square, collision_ratio,
                ns_per_hash, unique_hashes, total_collisions, bits_needed
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                run_id,
                table_size,
                metrics.get('golden_prime', 0),
                metrics.get('chi_square', 0),
                metrics.get('collision_ratio', 0),
                metrics.get('ns_per_hash', 0),
                metrics.get('unique_hashes', 0),
                metrics.get('total_collisions', 0),
                table_size.bit_length()
            ))
            collector.conn.commit()
            
            print(f"  ✓ Chi²: {metrics.get('chi_square', 0):.4f}, "
                  f"Collisions: {metrics.get('collision_ratio', 0):.4f}, "
                  f"Performance: {metrics.get('ns_per_hash', 0):.2f} ns/hash")
            print(f"  Time: {elapsed:.2f}s")

def run_smhasher_comparison(collector):
    """Run SMHasher tests on CROCS for comparison"""
    print(f"\n{'='*60}")
    print("Running SMHasher test suite on CROCS")
    print(f"{'='*60}")
    
    # Check if SMHasher is built
    smhasher_path = "third_party/smhasher.git/SMHasher"
    if not os.path.exists(f"build/{smhasher_path}"):
        print("Building SMHasher...")
        # Build SMHasher here if needed
        pass
    
    # Run basic SMHasher tests
    # This would integrate CROCS with SMHasher testing framework
    print("SMHasher integration pending...")

def main():
    # Parse arguments
    if len(sys.argv) > 1 and sys.argv[1] == "--quick":
        configs_to_run = ["small"]
        print("Running quick test (small sizes only)")
    else:
        configs_to_run = ["small", "medium", "powers_of_2", "primes"]
        if "--all" in sys.argv:
            configs_to_run.append("large")
    
    # Initialize collector
    collector = CROCSDataCollector()
    
    # Build test executable
    if not build_test_executable():
        print("Build failed, exiting")
        return 1
    
    # Run test batches
    start_time = time.time()
    
    for config_name in configs_to_run:
        if config_name in TEST_CONFIGS:
            run_test_batch(collector, config_name, TEST_CONFIGS[config_name])
            time.sleep(1)  # Brief pause between batches
    
    # Run SMHasher comparison if requested
    if "--smhasher" in sys.argv:
        run_smhasher_comparison(collector)
    
    # Summary for this run only
    total_time = time.time() - start_time
    
    # Get stats for just this session's runs
    cursor = collector.conn.cursor()
    run_ids = []
    for config_name in configs_to_run:
        if config_name in TEST_CONFIGS:
            cursor.execute("""
                SELECT run_id FROM test_runs 
                WHERE test_type = 'systematic_batch' 
                AND json_extract(parameters, '$.config_name') = ?
                ORDER BY timestamp DESC LIMIT 1
            """, (config_name,))
            row = cursor.fetchone()
            if row:
                run_ids.append(row[0])
    
    if run_ids:
        placeholders = ','.join(['?' for _ in run_ids])
        cursor.execute(f"""
            SELECT 
                COUNT(*) as total_tests,
                AVG(chi_square) as avg_chi_square,
                AVG(ns_per_hash) as avg_performance,
                SUM(CASE WHEN chi_square BETWEEN 0.95 AND 1.05 THEN 1 ELSE 0 END) as within_5pct
            FROM hash_tests
            WHERE run_id IN ({placeholders})
        """, run_ids)
        
        session_stats = dict(cursor.fetchone())
        
        print(f"\n{'='*60}")
        print("Testing Complete!")
        print(f"{'='*60}")
        print(f"Total time: {total_time/60:.1f} minutes")
        print(f"Tests run this session: {session_stats['total_tests']:,}")
        print(f"Average chi-square: {session_stats['avg_chi_square']:.4f}")
        print(f"Average performance: {session_stats['avg_performance']:.2f} ns/hash")
        
        if session_stats['total_tests'] > 0:
            pct = 100 * session_stats['within_5pct'] / session_stats['total_tests']
            print(f"Chi-square within 5%: {session_stats['within_5pct']}/{session_stats['total_tests']} "
                  f"({pct:.1f}%)")
    
    # Also show cumulative database stats
    print(f"\n{'='*60}")
    print("Cumulative Database Statistics:")
    print(f"{'='*60}")
    stats = collector.analyze_results()
    print(f"Total tests in database: {stats['overall']['total_tests']:,}")
    print(f"Overall avg chi-square: {stats['overall']['avg_chi_square']:.4f}")
    
    collector.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())