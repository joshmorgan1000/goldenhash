#!/usr/bin/env python3
"""
CROCS Systematic Data Collection System
Uses SQLite for data storage and analysis
"""

import sqlite3
import subprocess
import json
import datetime
import hashlib
import platform
import os
import sys
from typing import Dict, List, Any, Optional
import csv

class CROCSDataCollector:
    def __init__(self, db_path: str = "crocs_data.db"):
        self.db_path = db_path
        self.conn = sqlite3.connect(db_path)
        self.conn.row_factory = sqlite3.Row
        self.setup_database()
        
    def setup_database(self):
        """Create database schema"""
        cursor = self.conn.cursor()
        
        # Test runs table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS test_runs (
            run_id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            machine_info TEXT,
            test_type TEXT,
            parameters TEXT,
            notes TEXT
        )""")
        
        # Main results table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS hash_tests (
            test_id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER,
            table_size INTEGER,
            golden_prime INTEGER,
            chi_square REAL,
            collision_ratio REAL,
            avalanche_score REAL,
            distribution_uniformity REAL,
            ns_per_hash REAL,
            unique_hashes INTEGER,
            total_collisions INTEGER,
            max_bucket_load INTEGER,
            bits_needed INTEGER,
            prime_distance INTEGER,
            FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
        )""")
        
        # Performance comparison table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS performance_comparisons (
            comparison_id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER,
            table_size INTEGER,
            hash_function TEXT,
            ns_per_hash REAL,
            unique_hashes INTEGER,
            collisions INTEGER,
            chi_square REAL,
            FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
        )""")
        
        # Large scale tests table
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS large_scale_tests (
            test_id INTEGER PRIMARY KEY AUTOINCREMENT,
            run_id INTEGER,
            table_size INTEGER,
            size_description TEXT,
            golden_prime INTEGER,
            golden_ratio_error REAL,
            ns_per_hash REAL,
            bits_needed REAL,
            prime_found BOOLEAN,
            sample_collisions INTEGER,
            sample_size INTEGER,
            FOREIGN KEY (run_id) REFERENCES test_runs(run_id)
        )""")
        
        # Create indexes
        cursor.execute("CREATE INDEX IF NOT EXISTS idx_table_size ON hash_tests(table_size)")
        cursor.execute("CREATE INDEX IF NOT EXISTS idx_run_id ON hash_tests(run_id)")
        
        self.conn.commit()
    
    def create_test_run(self, test_type: str, parameters: Dict[str, Any] = None, notes: str = "") -> int:
        """Create a new test run entry"""
        machine_info = {
            'platform': platform.platform(),
            'processor': platform.processor(),
            'python_version': platform.python_version(),
            'machine': platform.machine()
        }
        
        cursor = self.conn.cursor()
        cursor.execute("""
        INSERT INTO test_runs (machine_info, test_type, parameters, notes)
        VALUES (?, ?, ?, ?)
        """, (json.dumps(machine_info), test_type, json.dumps(parameters or {}), notes))
        
        self.conn.commit()
        return cursor.lastrowid
    
    def import_csv_data(self, csv_path: str, run_id: int):
        """Import data from existing CSV file"""
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                cursor = self.conn.cursor()
                cursor.execute("""
                INSERT INTO hash_tests (
                    run_id, table_size, golden_prime, chi_square, collision_ratio,
                    avalanche_score, distribution_uniformity, ns_per_hash,
                    unique_hashes, total_collisions, max_bucket_load,
                    bits_needed, prime_distance
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    run_id,
                    int(row['table_size']),
                    int(row['golden_prime']),
                    float(row['chi_square']),
                    float(row['collision_ratio']),
                    float(row['avalanche_score']),
                    float(row['distribution_uniformity']),
                    float(row['ns_per_hash']),
                    int(row['unique_hashes']),
                    int(row['total_collisions']),
                    int(row['max_bucket_load']),
                    int(row['bits_needed']),
                    int(row['prime_distance_from_golden'])
                ))
        
        self.conn.commit()
        print(f"Imported {reader.line_num - 1} records from {csv_path}")
    
    def run_comprehensive_test(self, extra_params: List[str] = None) -> int:
        """Run comprehensive C++ test and collect results"""
        print("Running comprehensive CROCS test...")
        
        # Create test run
        run_id = self.create_test_run("comprehensive", 
                                      {"extra_params": extra_params})
        
        # Run the test
        cmd = ["./build/test_crocs_comprehensive"]
        if extra_params:
            cmd.extend(extra_params)
            
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Test failed: {result.stderr}")
            return run_id
        
        # Import the generated CSV
        csv_path = "build/crocs_test_results.csv"
        if os.path.exists(csv_path):
            self.import_csv_data(csv_path, run_id)
            # Move CSV to archive
            archive_path = f"data/run_{run_id}_comprehensive.csv"
            os.makedirs("data", exist_ok=True)
            os.rename(csv_path, archive_path)
        
        return run_id
    
    def run_large_scale_test(self) -> int:
        """Run large scale test and parse output"""
        print("Running large scale CROCS test...")
        
        run_id = self.create_test_run("large_scale")
        
        result = subprocess.run(["./build/test_large_scale"], 
                              capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"Test failed: {result.stderr}")
            return run_id
        
        # Parse output
        lines = result.stdout.split('\n')
        in_results = False
        
        for line in lines:
            if "Size Name" in line and "Prime" in line:
                in_results = True
                continue
            
            if in_results and line.strip() and not line.startswith("="):
                parts = line.split()
                if len(parts) >= 5:
                    try:
                        # Parse the line
                        size_desc = parts[0]
                        if parts[1].startswith('('):
                            size_desc += " " + parts[1]
                            prime_idx = 2
                        else:
                            prime_idx = 1
                        
                        cursor = self.conn.cursor()
                        cursor.execute("""
                        INSERT INTO large_scale_tests (
                            run_id, size_description, golden_prime,
                            golden_ratio_error, ns_per_hash
                        ) VALUES (?, ?, ?, ?, ?)
                        """, (
                            run_id,
                            size_desc,
                            int(parts[prime_idx]),
                            float(parts[prime_idx + 1].rstrip('%')),
                            float(parts[prime_idx + 2])
                        ))
                    except (ValueError, IndexError):
                        continue
        
        self.conn.commit()
        return run_id
    
    def analyze_results(self) -> Dict[str, Any]:
        """Analyze collected data"""
        cursor = self.conn.cursor()
        
        # Overall statistics
        cursor.execute("""
        SELECT 
            COUNT(DISTINCT run_id) as num_runs,
            COUNT(*) as total_tests,
            MIN(table_size) as min_size,
            MAX(table_size) as max_size,
            AVG(chi_square) as avg_chi_square,
            AVG(ns_per_hash) as avg_performance,
            AVG(collision_ratio) as avg_collision_ratio
        FROM hash_tests
        """)
        
        overall = dict(cursor.fetchone())
        
        # Chi-square quality
        cursor.execute("""
        SELECT 
            COUNT(*) as total,
            SUM(CASE WHEN chi_square BETWEEN 0.95 AND 1.05 THEN 1 ELSE 0 END) as within_5pct,
            SUM(CASE WHEN chi_square BETWEEN 0.90 AND 1.10 THEN 1 ELSE 0 END) as within_10pct
        FROM hash_tests
        """)
        
        chi_quality = dict(cursor.fetchone())
        
        # Performance by table size ranges
        cursor.execute("""
        SELECT 
            CASE 
                WHEN table_size < 1000 THEN 'Small (<1K)'
                WHEN table_size < 100000 THEN 'Medium (1K-100K)'
                WHEN table_size < 10000000 THEN 'Large (100K-10M)'
                ELSE 'Very Large (>10M)'
            END as size_range,
            AVG(ns_per_hash) as avg_performance,
            COUNT(*) as num_tests
        FROM hash_tests
        GROUP BY size_range
        ORDER BY MIN(table_size)
        """)
        
        perf_by_size = [dict(row) for row in cursor.fetchall()]
        
        return {
            'overall': overall,
            'chi_square_quality': chi_quality,
            'performance_by_size': perf_by_size
        }
    
    def export_for_latex(self, output_dir: str = "latex_data"):
        """Export data in formats suitable for LaTeX"""
        os.makedirs(output_dir, exist_ok=True)
        
        # Export summary statistics
        stats = self.analyze_results()
        with open(f"{output_dir}/summary_stats.json", 'w') as f:
            json.dump(stats, f, indent=2)
        
        # Export key data tables
        cursor = self.conn.cursor()
        
        # Table 1: Representative results across sizes
        cursor.execute("""
        SELECT table_size, golden_prime, chi_square, collision_ratio, 
               ns_per_hash, unique_hashes, total_collisions
        FROM hash_tests
        WHERE table_size IN (97, 1009, 10007, 100003, 1000003)
        ORDER BY table_size
        """)
        
        with open(f"{output_dir}/representative_results.csv", 'w') as f:
            writer = csv.writer(f)
            writer.writerow(['Table Size', 'Golden Prime', 'Chi-Square', 
                           'Collision Ratio', 'ns/hash', 'Unique', 'Collisions'])
            writer.writerows(cursor.fetchall())
        
        print(f"LaTeX data exported to {output_dir}/")
    
    def close(self):
        """Close database connection"""
        self.conn.close()


def main():
    """Main data collection pipeline"""
    collector = CROCSDataCollector()
    
    if len(sys.argv) > 1:
        command = sys.argv[1]
        
        if command == "import":
            # Import existing CSV
            if len(sys.argv) > 2:
                run_id = collector.create_test_run("imported", 
                                                  notes=f"Imported from {sys.argv[2]}")
                collector.import_csv_data(sys.argv[2], run_id)
            else:
                print("Usage: python data_collector.py import <csv_file>")
        
        elif command == "run":
            # Run comprehensive test
            collector.run_comprehensive_test()
        
        elif command == "large":
            # Run large scale test
            collector.run_large_scale_test()
        
        elif command == "analyze":
            # Analyze results
            results = collector.analyze_results()
            print("\n=== CROCS Data Analysis ===")
            print(f"Total runs: {results['overall']['num_runs']}")
            print(f"Total tests: {results['overall']['total_tests']}")
            print(f"Chi-square: {results['overall']['avg_chi_square']:.4f}")
            print(f"Performance: {results['overall']['avg_performance']:.2f} ns/hash")
            
            chi = results['chi_square_quality']
            print(f"\nChi-square quality:")
            print(f"  Within 5%: {chi['within_5pct']}/{chi['total']} "
                  f"({100*chi['within_5pct']/chi['total']:.1f}%)")
        
        elif command == "export":
            # Export for LaTeX
            collector.export_for_latex()
        
        else:
            print(f"Unknown command: {command}")
    
    else:
        print("CROCS Data Collector")
        print("Usage:")
        print("  python data_collector.py import <csv_file>  # Import existing CSV")
        print("  python data_collector.py run               # Run comprehensive test")
        print("  python data_collector.py large             # Run large scale test")
        print("  python data_collector.py analyze           # Analyze collected data")
        print("  python data_collector.py export            # Export for LaTeX")
    
    collector.close()


if __name__ == "__main__":
    main()