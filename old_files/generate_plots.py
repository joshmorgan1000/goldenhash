#!/usr/bin/env python3
"""
Generate plots for CROCS whitepaper
Uses matplotlib to create publication-quality figures
"""

import sqlite3
import matplotlib.pyplot as plt
import numpy as np
import os

# Set up matplotlib for publication
plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 12
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['figure.figsize'] = (6, 4)
plt.rcParams['savefig.dpi'] = 300
plt.rcParams['savefig.bbox'] = 'tight'

def load_data():
    """Load data from SQLite database"""
    conn = sqlite3.connect('crocs_data.db')
    conn.row_factory = sqlite3.Row
    return conn

def plot_chi_square_distribution(conn, output_dir):
    """Plot chi-square distribution"""
    cursor = conn.cursor()
    cursor.execute("""
    SELECT table_size, chi_square 
    FROM hash_tests 
    ORDER BY table_size
    """)
    
    data = cursor.fetchall()
    sizes = [row[0] for row in data]
    chi_squares = [row[1] for row in data]
    
    plt.figure(figsize=(8, 5))
    plt.scatter(np.log10(sizes), chi_squares, alpha=0.7, s=50)
    plt.axhline(y=1.0, color='r', linestyle='--', label='Ideal (1.0)')
    plt.axhline(y=0.95, color='g', linestyle=':', alpha=0.5, label='±5% bounds')
    plt.axhline(y=1.05, color='g', linestyle=':', alpha=0.5)
    
    plt.xlabel('log₁₀(Table Size)')
    plt.ylabel('Chi-Square Value (normalized)')
    plt.title('Chi-Square Distribution Quality vs Table Size')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.savefig(f'{output_dir}/chi_square_distribution.pdf')
    plt.close()

def plot_performance_scaling(conn, output_dir):
    """Plot performance scaling"""
    cursor = conn.cursor()
    cursor.execute("""
    SELECT table_size, ns_per_hash 
    FROM hash_tests 
    ORDER BY table_size
    """)
    
    data = cursor.fetchall()
    sizes = [row[0] for row in data]
    performance = [row[1] for row in data]
    
    plt.figure(figsize=(8, 5))
    
    # Separate powers of 2 and primes
    powers_of_2 = [(s, p) for s, p in zip(sizes, performance) if (s & (s-1)) == 0]
    primes = [(s, p) for s, p in zip(sizes, performance) if (s & (s-1)) != 0]
    
    if powers_of_2:
        p2_sizes, p2_perf = zip(*powers_of_2)
        plt.scatter(np.log10(p2_sizes), p2_perf, alpha=0.7, s=50, 
                   label='Powers of 2', marker='s')
    
    if primes:
        pr_sizes, pr_perf = zip(*primes)
        plt.scatter(np.log10(pr_sizes), pr_perf, alpha=0.7, s=50, 
                   label='Prime sizes', marker='o')
    
    # Add trend line
    log_sizes = np.log10(sizes)
    z = np.polyfit(log_sizes, performance, 1)
    p = np.poly1d(z)
    plt.plot(log_sizes, p(log_sizes), 'r--', alpha=0.5, 
             label=f'Trend: {z[0]:.2f}x + {z[1]:.2f}')
    
    plt.xlabel('log₁₀(Table Size)')
    plt.ylabel('Nanoseconds per Hash')
    plt.title('Hash Performance Scaling')
    plt.legend()
    plt.grid(True, alpha=0.3)
    
    plt.savefig(f'{output_dir}/performance_scaling.pdf')
    plt.close()

def plot_collision_accuracy(conn, output_dir):
    """Plot collision prediction accuracy"""
    cursor = conn.cursor()
    cursor.execute("""
    SELECT table_size, collision_ratio 
    FROM hash_tests 
    ORDER BY table_size
    """)
    
    data = cursor.fetchall()
    sizes = [row[0] for row in data]
    ratios = [row[1] for row in data]
    
    plt.figure(figsize=(8, 5))
    plt.scatter(np.log10(sizes), ratios, alpha=0.7, s=50)
    plt.axhline(y=1.0, color='r', linestyle='--', label='Expected')
    plt.axhline(y=0.95, color='g', linestyle=':', alpha=0.5, label='±5% bounds')
    plt.axhline(y=1.05, color='g', linestyle=':', alpha=0.5)
    
    plt.xlabel('log₁₀(Table Size)')
    plt.ylabel('Actual/Expected Collisions')
    plt.title('Collision Prediction Accuracy')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.ylim(0.9, 1.1)
    
    plt.savefig(f'{output_dir}/collision_accuracy.pdf')
    plt.close()

def plot_golden_ratio_accuracy(conn, output_dir):
    """Plot golden ratio prime selection accuracy"""
    cursor = conn.cursor()
    cursor.execute("""
    SELECT table_size, golden_prime, prime_distance 
    FROM hash_tests 
    ORDER BY table_size
    """)
    
    data = cursor.fetchall()
    sizes = [row[0] for row in data]
    primes = [row[1] for row in data]
    distances = [abs(row[2]) for row in data]
    
    # Calculate actual ratios
    phi = 1.6180339887498948482
    ratios = [s/p for s, p in zip(sizes, primes)]
    errors = [abs(r - phi)/phi * 100 for r in ratios]
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8))
    
    # Top plot: Ratio accuracy
    ax1.scatter(np.log10(sizes), ratios, alpha=0.7, s=50)
    ax1.axhline(y=phi, color='r', linestyle='--', label=f'φ = {phi:.6f}')
    ax1.set_xlabel('log₁₀(Table Size)')
    ax1.set_ylabel('N / Prime')
    ax1.set_title('Golden Ratio Accuracy')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Bottom plot: Distance from ideal
    ax2.scatter(np.log10(sizes), distances, alpha=0.7, s=50)
    ax2.set_xlabel('log₁₀(Table Size)')
    ax2.set_ylabel('|Prime - N/φ|')
    ax2.set_title('Prime Selection Distance')
    ax2.set_yscale('log')
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/golden_ratio_accuracy.pdf')
    plt.close()

def generate_summary_table(conn, output_dir):
    """Generate LaTeX summary table"""
    cursor = conn.cursor()
    cursor.execute("""
    SELECT table_size, golden_prime, chi_square, collision_ratio, ns_per_hash
    FROM hash_tests
    WHERE table_size IN (97, 1009, 10007, 100003, 1000003, 8000009)
    ORDER BY table_size
    """)
    
    with open(f'{output_dir}/summary_table.tex', 'w') as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\caption{Representative CROCS test results}\n")
        f.write("\\begin{tabular}{@{}rrrrr@{}}\n")
        f.write("\\toprule\n")
        f.write("Table Size & Golden Prime & Chi-Square & Collision Ratio & ns/hash \\\\\n")
        f.write("\\midrule\n")
        
        for row in cursor.fetchall():
            f.write(f"{row[0]:,} & {row[1]:,} & {row[2]:.4f} & {row[3]:.4f} & {row[4]:.2f} \\\\\n")
        
        f.write("\\bottomrule\n")
        f.write("\\end{tabular}\n")
        f.write("\\end{table}\n")

def main():
    output_dir = 'docs/figures'
    os.makedirs(output_dir, exist_ok=True)
    
    conn = load_data()
    
    print("Generating plots...")
    plot_chi_square_distribution(conn, output_dir)
    print("  ✓ Chi-square distribution")
    
    plot_performance_scaling(conn, output_dir)
    print("  ✓ Performance scaling")
    
    plot_collision_accuracy(conn, output_dir)
    print("  ✓ Collision accuracy")
    
    plot_golden_ratio_accuracy(conn, output_dir)
    print("  ✓ Golden ratio accuracy")
    
    generate_summary_table(conn, output_dir)
    print("  ✓ Summary table")
    
    conn.close()
    print(f"\nAll plots saved to {output_dir}/")

if __name__ == "__main__":
    main()