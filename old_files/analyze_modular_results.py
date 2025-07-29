#!/usr/bin/env python3
"""
Comprehensive analysis of modular hash data
"""

import sqlite3
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from collections import defaultdict
import math

def is_prime(n):
    """Check if a number is prime"""
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

def analyze_modular_data(db_path="modular_hash_data.db"):
    conn = sqlite3.connect(db_path)
    
    # Get all data
    cursor = conn.execute("""
        SELECT table_size, is_prime, avalanche_score, chi_square, collision_ratio,
               unique_hashes, total_collisions, expected_collisions,
               prime_high, prime_low, working_modulus
        FROM modular_hash_results
        ORDER BY table_size
    """)
    
    data = cursor.fetchall()
    
    # Separate by prime/composite
    prime_data = []
    composite_data = []
    
    for row in data:
        if row[1]:  # is_prime
            prime_data.append(row)
        else:
            composite_data.append(row)
    
    print("Modular Hash Analysis Results")
    print("=" * 60)
    print(f"Total table sizes tested: {len(data)}")
    print(f"Prime table sizes: {len(prime_data)}")
    print(f"Composite table sizes: {len(composite_data)}")
    print()
    
    # Overall statistics
    avalanche_scores = [row[2] for row in data]
    chi_squares = [row[3] for row in data]
    collision_ratios = [row[4] for row in data]
    
    print("Overall Statistics:")
    print(f"Avalanche: mean={np.mean(avalanche_scores):.4f}, std={np.std(avalanche_scores):.4f}")
    print(f"           min={np.min(avalanche_scores):.4f}, max={np.max(avalanche_scores):.4f}")
    print(f"Chi-square: mean={np.mean(chi_squares):.4f}, std={np.std(chi_squares):.4f}")
    print(f"            min={np.min(chi_squares):.4f}, max={np.max(chi_squares):.4f}")
    print(f"Collision ratio: mean={np.mean(collision_ratios):.4f}, std={np.std(collision_ratios):.4f}")
    print(f"                 min={np.min(collision_ratios):.4f}, max={np.max(collision_ratios):.4f}")
    print()
    
    # Compare prime vs composite
    if prime_data:
        prime_avalanche = [row[2] for row in prime_data]
        prime_chi = [row[3] for row in prime_data]
        prime_collision = [row[4] for row in prime_data]
        
        print("Prime Table Sizes:")
        print(f"Avalanche: mean={np.mean(prime_avalanche):.4f}, std={np.std(prime_avalanche):.4f}")
        print(f"Chi-square: mean={np.mean(prime_chi):.4f}, std={np.std(prime_chi):.4f}")
        print(f"Collision ratio: mean={np.mean(prime_collision):.4f}, std={np.std(prime_collision):.4f}")
        print()
    
    if composite_data:
        comp_avalanche = [row[2] for row in composite_data]
        comp_chi = [row[3] for row in composite_data]
        comp_collision = [row[4] for row in composite_data]
        
        print("Composite Table Sizes:")
        print(f"Avalanche: mean={np.mean(comp_avalanche):.4f}, std={np.std(comp_avalanche):.4f}")
        print(f"Chi-square: mean={np.mean(comp_chi):.4f}, std={np.std(comp_chi):.4f}")
        print(f"Collision ratio: mean={np.mean(comp_collision):.4f}, std={np.std(comp_collision):.4f}")
        print()
    
    # Quality thresholds
    good_avalanche = sum(1 for x in avalanche_scores if 0.45 <= x <= 0.55)
    good_chi = sum(1 for x in chi_squares if 0.9 <= x <= 1.1)
    good_collision = sum(1 for x in collision_ratios if x <= 1.2)
    
    print("Quality Analysis (percentage meeting criteria):")
    print(f"Good avalanche (0.45-0.55): {good_avalanche/len(data)*100:.1f}%")
    print(f"Good chi-square (0.9-1.1): {good_chi/len(data)*100:.1f}%")
    print(f"Good collision ratio (≤1.2): {good_collision/len(data)*100:.1f}%")
    print()
    
    # Find problematic sizes
    problems = []
    for row in data:
        table_size, is_prime, avalanche, chi, collision = row[:5]
        issues = []
        if avalanche < 0.3 or avalanche > 0.7:
            issues.append(f"avalanche={avalanche:.3f}")
        if chi < 0.5 or chi > 2.0:
            issues.append(f"chi={chi:.3f}")
        if collision > 2.0:
            issues.append(f"collision={collision:.3f}")
        
        if issues:
            problems.append((table_size, is_prime, ", ".join(issues)))
    
    if problems:
        print(f"Problematic table sizes ({len(problems)} total):")
        for i, (size, is_prime, issues) in enumerate(problems[:10]):
            prime_str = "prime" if is_prime else "composite"
            print(f"  {size} ({prime_str}): {issues}")
        if len(problems) > 10:
            print(f"  ... and {len(problems)-10} more")
    
    # Analyze patterns
    print("\nPattern Analysis:")
    
    # Group by table size ranges
    ranges = [(10000, 20000), (20000, 30000), (30000, 40000), (40000, 50000), (50000, 60000), (60000, 70000), (70000, 80000)]
    
    for start, end in ranges:
        range_data = [row for row in data if start <= row[0] < end]
        if range_data:
            range_avalanche = [row[2] for row in range_data]
            print(f"Range {start}-{end}: {len(range_data)} sizes, avalanche mean={np.mean(range_avalanche):.4f}")
    
    # Create visualizations
    create_plots(data, prime_data, composite_data)
    
    conn.close()

def create_plots(data, prime_data, composite_data):
    """Create visualization plots"""
    plt.style.use('seaborn-v0_8-darkgrid')
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    
    table_sizes = [row[0] for row in data]
    avalanche_scores = [row[2] for row in data]
    chi_squares = [row[3] for row in data]
    collision_ratios = [row[4] for row in data]
    
    # 1. Avalanche score over table size
    ax1 = axes[0, 0]
    ax1.scatter(table_sizes, avalanche_scores, alpha=0.5, s=1)
    ax1.axhline(y=0.5, color='r', linestyle='--', label='Ideal (0.5)')
    ax1.axhline(y=0.45, color='orange', linestyle=':', label='Good range')
    ax1.axhline(y=0.55, color='orange', linestyle=':')
    ax1.set_xlabel('Table Size')
    ax1.set_ylabel('Avalanche Score')
    ax1.set_title('Avalanche Score vs Table Size')
    ax1.legend()
    
    # 2. Chi-square distribution
    ax2 = axes[0, 1]
    ax2.hist(chi_squares, bins=50, alpha=0.7, edgecolor='black')
    ax2.axvline(x=1.0, color='r', linestyle='--', label='Ideal (1.0)')
    ax2.set_xlabel('Chi-square')
    ax2.set_ylabel('Count')
    ax2.set_title('Chi-square Distribution')
    ax2.legend()
    
    # 3. Prime vs Composite comparison
    ax3 = axes[1, 0]
    if prime_data and composite_data:
        prime_avalanche = [row[2] for row in prime_data]
        comp_avalanche = [row[2] for row in composite_data]
        
        ax3.violinplot([prime_avalanche, comp_avalanche], positions=[1, 2], showmeans=True)
        ax3.set_xticks([1, 2])
        ax3.set_xticklabels(['Prime', 'Composite'])
        ax3.set_ylabel('Avalanche Score')
        ax3.set_title('Avalanche Score: Prime vs Composite')
        ax3.axhline(y=0.5, color='r', linestyle='--', alpha=0.5)
    
    # 4. Collision ratio scatter
    ax4 = axes[1, 1]
    colors = ['red' if row[1] else 'blue' for row in data]
    ax4.scatter(table_sizes, collision_ratios, c=colors, alpha=0.5, s=1)
    ax4.axhline(y=1.0, color='g', linestyle='--', label='Ideal (1.0)')
    ax4.set_xlabel('Table Size')
    ax4.set_ylabel('Collision Ratio')
    ax4.set_title('Collision Ratio vs Table Size (red=prime, blue=composite)')
    ax4.set_ylim(0, 3)
    ax4.legend()
    
    plt.tight_layout()
    plt.savefig('modular_hash_analysis.png', dpi=300)
    print("\nPlots saved to modular_hash_analysis.png")
    
    # Additional detailed avalanche plot
    plt.figure(figsize=(12, 8))
    
    # Sort by avalanche score for better visualization
    sorted_data = sorted(data, key=lambda x: x[2])
    sorted_sizes = [row[0] for row in sorted_data]
    sorted_avalanche = [row[2] for row in sorted_data]
    sorted_isprime = [row[1] for row in sorted_data]
    
    # Color by prime/composite
    colors = ['red' if p else 'blue' for p in sorted_isprime]
    
    plt.scatter(range(len(sorted_data)), sorted_avalanche, c=colors, alpha=0.6, s=2)
    plt.axhline(y=0.5, color='green', linestyle='--', linewidth=2, label='Ideal (0.5)')
    plt.axhline(y=0.45, color='orange', linestyle=':', label='Good range')
    plt.axhline(y=0.55, color='orange', linestyle=':')
    
    plt.xlabel('Table Size Index (sorted by avalanche score)')
    plt.ylabel('Avalanche Score')
    plt.title('Avalanche Scores Sorted (red=prime, blue=composite)')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('avalanche_sorted.png', dpi=300)
    print("Avalanche plot saved to avalanche_sorted.png")

if __name__ == "__main__":
    analyze_modular_data()