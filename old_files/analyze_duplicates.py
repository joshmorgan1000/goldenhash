#!/usr/bin/env python3
"""
Analyze duplicate H("test") values in the collected data
"""

import sqlite3
from collections import defaultdict

def analyze_duplicates(db_path="modular_hash_data.db"):
    conn = sqlite3.connect(db_path)
    
    # Get all results ordered by table_size
    cursor = conn.execute("""
        SELECT table_size, is_prime, prime_high, prime_low, 
               working_modulus, test_hash, factors
        FROM modular_hash_results
        ORDER BY table_size
    """)
    
    # Group by test_hash
    hash_groups = defaultdict(list)
    
    for row in cursor:
        table_size, is_prime, prime_high, prime_low, working_mod, test_hash, factors = row
        hash_groups[test_hash].append({
            'table_size': table_size,
            'is_prime': is_prime,
            'prime_high': prime_high,
            'prime_low': prime_low,
            'working_mod': working_mod,
            'factors': factors
        })
    
    # Find duplicates
    print("Duplicate H(\"test\") Analysis")
    print("=" * 60)
    
    duplicates = {h: tables for h, tables in hash_groups.items() if len(tables) > 1}
    
    print(f"Total unique H(\"test\") values: {len(hash_groups)}")
    print(f"Duplicate H(\"test\") values: {len(duplicates)}")
    print()
    
    # Analyze patterns in duplicates
    consecutive_prime_dups = 0
    same_working_mod = 0
    
    for test_hash, tables in sorted(duplicates.items(), key=lambda x: x[1][0]['table_size']):
        print(f"\nH(\"test\") = {test_hash} appears {len(tables)} times:")
        
        # Check if consecutive primes
        sizes = [t['table_size'] for t in tables]
        all_prime = all(t['is_prime'] for t in tables)
        consecutive = all(sizes[i+1] - sizes[i] <= 10 for i in range(len(sizes)-1))
        
        if all_prime and consecutive:
            consecutive_prime_dups += 1
            
        # Check if same working modulus
        working_mods = [t['working_mod'] for t in tables]
        if len(set(working_mods)) == 1:
            same_working_mod += 1
        
        for t in tables:
            prime_indicator = "PRIME" if t['is_prime'] else "composite"
            print(f"  N={t['table_size']} ({prime_indicator}): "
                  f"working_mod={t['working_mod']}, "
                  f"primes=({t['prime_high']}, {t['prime_low']}), "
                  f"factors={t['factors']}")
    
    print(f"\nPattern Analysis:")
    print(f"Consecutive prime duplicates: {consecutive_prime_dups}")
    print(f"Same working modulus: {same_working_mod}")
    
    # Check specific hypothesis about N and N+2 both prime
    cursor = conn.execute("""
        SELECT a.table_size, a.test_hash, b.table_size, b.test_hash
        FROM modular_hash_results a
        JOIN modular_hash_results b ON b.table_size = a.table_size + 2
        WHERE a.is_prime = 1 AND b.is_prime = 1 
        AND a.test_hash = b.test_hash
    """)
    
    twin_prime_dups = cursor.fetchall()
    if twin_prime_dups:
        print(f"\nTwin prime duplicates (N and N+2 both prime with same hash):")
        for row in twin_prime_dups:
            print(f"  {row[0]} and {row[2]} -> H(\"test\") = {row[1]}")
    
    conn.close()

if __name__ == "__main__":
    analyze_duplicates()