#!/usr/bin/env python3
"""
Single CROCS test runner script
Handles data collection, database storage, and analysis
"""

import subprocess
import sqlite3
import sys
import time
import os

class CROCSTestRunner:
    def __init__(self, db_path="crocs_results.db"):
        self.db_path = db_path
        self.setup_database()
        
    def setup_database(self):
        """Create database schema if needed"""
        conn = sqlite3.connect(self.db_path)
        conn.execute("""
        CREATE TABLE IF NOT EXISTS test_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            table_size INTEGER,
            prime INTEGER,
            chi_square REAL,
            collisions INTEGER,
            unique_hashes INTEGER,
            ns_per_hash REAL,
            total_seconds REAL,
            num_operations INTEGER,
            data_size INTEGER
        )""")
        conn.commit()
        conn.close()
    
    def run_single_test(self, size, ops=1000000, data_size=64):
        """Run a single test and return results"""
        cmd = [
            "./crocs_test",
            "--size", str(size),
            "--ops", str(ops),
            "--data-size", str(data_size),
            "--csv"
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
            if result.returncode == 0 and result.stdout:
                # Parse CSV: size,prime,chi_square,collisions,unique,ns_per_hash,seconds
                parts = result.stdout.strip().split(',')
                if len(parts) >= 7:
                    return {
                        'table_size': int(parts[0]),
                        'prime': int(parts[1]),
                        'chi_square': float(parts[2]),
                        'collisions': int(parts[3]),
                        'unique_hashes': int(parts[4]),
                        'ns_per_hash': float(parts[5]),
                        'total_seconds': float(parts[6]),
                        'num_operations': ops,
                        'data_size': data_size
                    }
        except subprocess.TimeoutExpired:
            print(f"Timeout for size {size}")
        except Exception as e:
            print(f"Error for size {size}: {e}")
        
        return None
    
    def save_result(self, result):
        """Save test result to database"""
        conn = sqlite3.connect(self.db_path)
        conn.execute("""
        INSERT INTO test_results 
        (table_size, prime, chi_square, collisions, unique_hashes, 
         ns_per_hash, total_seconds, num_operations, data_size)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            result['table_size'], result['prime'], result['chi_square'],
            result['collisions'], result['unique_hashes'], result['ns_per_hash'],
            result['total_seconds'], result['num_operations'], result['data_size']
        ))
        conn.commit()
        conn.close()
    
    def run_test_suite(self, test_plan):
        """Run a suite of tests based on test plan"""
        total = len(test_plan)
        print(f"Running {total} tests...")
        print("=" * 60)
        
        start_time = time.time()
        
        for i, (size, ops) in enumerate(test_plan):
            print(f"[{i+1}/{total}] Size {size:,} with {ops:,} ops... ", end='', flush=True)
            
            test_start = time.time()
            result = self.run_single_test(size, ops)
            test_time = time.time() - test_start
            
            if result:
                self.save_result(result)
                print(f"✓ ({test_time:.1f}s, χ²={result['chi_square']:.4f})")
            else:
                print(f"✗ ({test_time:.1f}s)")
            
            # Progress estimate
            elapsed = time.time() - start_time
            if i > 0:
                avg_time = elapsed / (i + 1)
                remaining = avg_time * (total - i - 1)
                print(f"  Progress: {100*(i+1)/total:.1f}%, ETA: {remaining/60:.1f} min")
        
        total_time = time.time() - start_time
        print(f"\nCompleted in {total_time/60:.1f} minutes")
    
    def analyze_results(self):
        """Analyze collected results"""
        conn = sqlite3.connect(self.db_path)
        
        # Overall statistics
        stats = conn.execute("""
        SELECT 
            COUNT(*) as count,
            AVG(chi_square) as avg_chi,
            MIN(chi_square) as min_chi,
            MAX(chi_square) as max_chi,
            AVG(ns_per_hash) as avg_perf
        FROM test_results
        """).fetchone()
        
        print("\nOverall Statistics:")
        print(f"Total tests: {stats[0]}")
        print(f"Chi-square: avg={stats[1]:.4f}, min={stats[2]:.4f}, max={stats[3]:.4f}")
        print(f"Performance: {stats[4]:.2f} ns/hash")
        
        conn.close()

def generate_test_plan():
    """Generate comprehensive test plan"""
    plan = []
    
    # Small tables with many operations
    for size in [97, 997, 9973, 99991]:
        plan.append((size, size * 10000))  # 10,000x table size
    
    # Powers of 2 region
    for p in range(10, 25):
        size = 2**p
        ops = min(size * 1000, 100000000)  # Cap at 100M
        plan.append((size, ops))
    
    # Large primes
    large_primes = [1000003, 10000019, 100000007, 1000000007]
    for size in large_primes:
        ops = min(size * 100, 1000000000)  # Cap at 1B
        plan.append((size, ops))
    
    # Huge sizes (sampling mode)
    huge_sizes = [2**30 - 1, 2**35 - 1, 2**40 - 1]
    for size in huge_sizes:
        plan.append((size, 10000000))  # Fixed 10M ops
    
    return plan

def main():
    if len(sys.argv) < 2:
        print("Usage: python run_tests.py [command]")
        print("Commands:")
        print("  quick     - Quick test (few sizes)")
        print("  standard  - Standard test suite")
        print("  extreme   - Extreme test (hours)")
        print("  analyze   - Analyze results")
        sys.exit(1)
    
    runner = CROCSTestRunner()
    command = sys.argv[1]
    
    if command == "quick":
        plan = [(10007, 1000000), (100003, 10000000), (1000003, 10000000)]
        runner.run_test_suite(plan)
        
    elif command == "standard":
        plan = generate_test_plan()
        runner.run_test_suite(plan)
        
    elif command == "extreme":
        # Generate massive test plan
        plan = []
        for size in range(1000, 1000000, 10000):
            if size < 10000:
                ops = size * 10000
            elif size < 100000:
                ops = size * 1000
            else:
                ops = size * 100
            plan.append((size, ops))
        
        print(f"Extreme mode: {len(plan)} tests")
        confirm = input("This will take HOURS. Continue? (yes/no): ")
        if confirm.lower() == "yes":
            runner.run_test_suite(plan)
            
    elif command == "analyze":
        runner.analyze_results()
        
    else:
        print(f"Unknown command: {command}")

if __name__ == "__main__":
    main()