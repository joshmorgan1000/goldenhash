#!/usr/bin/env python3
"""
Collect modular rotation hash data for all N from 10,000 to 16,787,216
Runs enough tests to expect ~10 collisions for each N
"""

import subprocess
import json
import sqlite3
import time
import math
import sys
import tqdm

def calculate_tests_for_collisions(n, target_collisions=10):
    """
    Calculate number of tests needed to expect target_collisions
    Using birthday paradox: E[collisions] = n - m*(1 - e^(-n/m))
    Where n = number of tests, m = table size
    
    Approximation: for small collision rates, we need about sqrt(2*m*target_collisions)
    """
    # For more accurate calculation, use iterative approach
    if target_collisions >= n:
        return n  # Can't have more collisions than table size
    
    # Initial estimate using approximation
    tests = int(math.sqrt(2 * n * target_collisions))
    
    # Refine using actual formula
    for _ in range(10):  # Iterate to converge
        expected_unique = n * (1 - math.exp(-tests / n))
        expected_collisions = tests - expected_unique
        
        if expected_collisions < target_collisions * 0.9:
            tests = int(tests * 1.1)
        elif expected_collisions > target_collisions * 1.1:
            tests = int(tests * 0.95)
        else:
            break
    
    # Ensure minimum reasonable test count
    return max(tests, 100)

class ModularDataCollector:
    def __init__(self, db_path="modular_hash_data.db"):
        self.db_path = db_path
        self.setup_database()
        
    def setup_database(self):
        """Create database schema"""
        conn = sqlite3.connect(self.db_path)
        conn.execute("""
        CREATE TABLE IF NOT EXISTS modular_hash_results (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            table_size INTEGER,
            is_prime BOOLEAN,
            prime_high INTEGER,
            prime_low INTEGER,
            working_modulus INTEGER,
            num_tests INTEGER,
            unique_hashes INTEGER,
            total_collisions INTEGER,
            expected_collisions REAL,
            collision_ratio REAL,
            chi_square REAL,
            avalanche_score REAL,
            max_bucket_load INTEGER,
            test_hash INTEGER,
            performance_ns REAL,
            factors TEXT
        )""")
        
        # Create index for faster lookups
        conn.execute("CREATE INDEX IF NOT EXISTS idx_table_size ON modular_hash_results(table_size)")
        conn.commit()
        conn.close()
    
    def run_test(self, table_size, num_tests):
        """Run modulo_rotate and capture results"""
        cmd = ["../build/goldenhash_test", str(table_size), str(num_tests), "--json"]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            if result.returncode == 0 and result.stdout:
                return json.loads(result.stdout)
            else:
                print(f"Error running test for N={table_size}: {result.stderr}")
                print(f"Command was: {' '.join(cmd)}")
                print(f"Output was: {result.stdout}")
                return None
        except subprocess.TimeoutExpired:
            print(f"Timeout for N={table_size}")
            return None
        except json.JSONDecodeError as e:
            print(f"JSON parse error for N={table_size}: {e}")
            print(f"Output was: {result.stdout}")
            return None
        except Exception as e:
            print(f"Unexpected error for N={table_size}: {e}")
            return None
    
    def save_result(self, data):
        """Save test result to database"""
        conn = sqlite3.connect(self.db_path)
        
        # Handle is_prime field (might be string "true"/"false" or boolean)
        is_prime = data.get("is_prime", False)
        if isinstance(is_prime, str):
            is_prime = is_prime.lower() == "true"
        
        conn.execute("""
        INSERT INTO modular_hash_results 
        (table_size, is_prime, prime_high, prime_low, working_modulus,
         num_tests, unique_hashes, total_collisions, expected_collisions,
         collision_ratio, chi_square, avalanche_score, max_bucket_load,
         test_hash, performance_ns, factors)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            data["table_size"],
            is_prime,
            data["prime_high"],
            data["prime_low"],
            data["working_modulus"],
            data.get("num_tests", 0),  # In case it's missing
            data["unique_hashes"],
            data["total_collisions"],
            data["expected_collisions"],
            data["collision_ratio"],
            data["chi_square"],
            data["avalanche_score"],
            data["max_bucket_load"],
            data["test_vectors"]["abc"] if "abc" in data.get("test_vectors", {}) else 0,  # Use "abc" as the test hash
            data["performance_ns_per_hash"],
            data["factors"]
        ))
        
        conn.commit()
        conn.close()
    
    def table_size_exists(self, table_size):
        """Check if we already have data for this table size"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.execute(
            "SELECT COUNT(*) FROM modular_hash_results WHERE table_size = ?",
            (table_size,)
        )
        count = cursor.fetchone()[0]
        conn.close()
        return count > 0

def main():
    print("Modular Hash Data Collection")
    print("=" * 60)
    print(f"Range: 10,000 to 75535")
    print(f"Target: ~10 collisions per table size")
    print("=" * 60)
    
    collector = ModularDataCollector()
    
    # Define range
    start_n = 0
    end_n = 65535 * 16
    
    # Count total and estimate time
    total_count = end_n - start_n + 1
    print(f"Total table sizes to test: {total_count:,}")
    
    # Option to resume or start fresh
    if len(sys.argv) > 1 and sys.argv[1] == "--resume":
        print("Resuming from last position...")
    else:
        response = input("Start fresh? This will take many hours. (yes/no): ")
        if response.lower() != "yes":
            print("Aborted.")
            return
    
    completed = 0
    skipped = 0
    start_time = time.time()

    # The golden ratio
    GOLDEN_RATIO = (1 + math.sqrt(5)) / 2
    
    # Test each N
    #for n in range(start_n, end_n + 6):
    for num in tqdm.tqdm(range(start_n, end_n), unit="N"):
        # n = (num * 6) + 1679616
        n = num + 67536 + 65535
        # Skip if already exists (for resume functionality)
        if collector.table_size_exists(n):
            skipped += 1
            continue
        
        # Calculate tests needed for ~10 collisions
        num_tests = calculate_tests_for_collisions(n, target_collisions=10)
        
        # Run test
        test_start = time.time()
        result = collector.run_test(n, num_tests)
        test_time = time.time() - test_start
        
        if result is None:
            # Exit if the test failed
            print(f"Failed to run test for N={n}, exiting.")
            sys.exit(1)

        if result:
            collector.save_result(result)
            completed += 1
        else:
            print(f" ✗ ({test_time:.1f}s)")
    
    # Final summary
    total_time = time.time() - start_time
    print(f"\n{'='*60}")
    print(f"Collection complete!")
    print(f"Total time: {total_time/3600:.1f} hours")
    print(f"Completed: {completed}")
    print(f"Skipped: {skipped}")
    print(f"Average rate: {completed/(total_time+0.01):.2f} tests/second")
    
    # Quick analysis
    conn = sqlite3.connect(collector.db_path)
    cursor = conn.execute("""
        SELECT 
            AVG(chi_square) as avg_chi,
            AVG(avalanche_score) as avg_avalanche,
            AVG(collision_ratio) as avg_collision_ratio,
            MIN(chi_square) as min_chi,
            MAX(chi_square) as max_chi
        FROM modular_hash_results
    """)
    
    stats = cursor.fetchone()
    if stats[0]:
        print(f"\nStatistics:")
        print(f"Chi-square: {stats[0]:.4f} (min: {stats[3]:.4f}, max: {stats[4]:.4f})")
        print(f"Avalanche: {stats[1]:.4f}")
        print(f"Collision ratio: {stats[2]:.4f}")
    
    conn.close()

if __name__ == "__main__":
    main()