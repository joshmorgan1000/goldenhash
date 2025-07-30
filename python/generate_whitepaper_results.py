#!/usr/bin/env python3
"""
Generate whitepaper results by running goldenhash tests and collecting data
"""

import subprocess
import json
import os
import sys
import numpy as np
from pathlib import Path
import pandas as pd

def run_hash_test(table_size, num_tests=10000):
    """Run the goldenhash_test binary and get JSON output"""
    binary_path = Path(__file__).parent.parent / "build" / "goldenhash_test"
    
    if not binary_path.exists():
        print(f"Error: Binary not found at {binary_path}")
        print("Please build the project first: cd build && make goldenhash_test")
        return None
    
    try:
        # Run with --json flag to get JSON output
        result = subprocess.run(
            [str(binary_path), str(table_size), str(num_tests), "--json"],
            capture_output=True,
            text=True,
            check=True
        )
        
        # Parse JSON output
        return json.loads(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"Error running test for table_size={table_size}: {e}")
        print(f"stderr: {e.stderr}")
        return None
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON for table_size={table_size}: {e}")
        print(f"stdout: {result.stdout}")
        return None

def is_prime(n):
    """Check if a number is prime"""
    if n < 2:
        return False
    if n == 2:
        return True
    if n % 2 == 0:
        return False
    for i in range(3, int(n**0.5) + 1, 2):
        if n % i == 0:
            return False
    return True

def generate_test_sizes():
    """Generate a variety of test sizes for comprehensive analysis"""
    sizes = []
    
    # Powers of 2 (from 2^8 to 2^24)
    for i in range(8, 25):
        sizes.append(2**i)
    
    # Powers of 2 minus 1 (Mersenne numbers)
    for i in range(8, 25):
        sizes.append(2**i - 1)
    
    # Powers of 2 plus 1
    for i in range(8, 25):
        sizes.append(2**i + 1)
    
    # Common prime sizes
    primes = [257, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 
              131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593]
    sizes.extend(primes)
    
    # Highly composite numbers
    composites = [360, 720, 1260, 2520, 5040, 10080, 20160, 40320, 83160, 
                  181440, 362880, 665280, 1330560, 2661120]
    sizes.extend(composites)
    
    # Generate many random sizes across different ranges
    import random
    random.seed(42)
    
    # Small range (256 to 1K)
    for _ in range(200):
        sizes.append(random.randint(256, 1024))
    
    # Medium range (1K to 64K)
    for _ in range(1000):
        sizes.append(random.randint(1024, 65536))
    
    # Large range (64K to 1M)
    for _ in range(500):
        sizes.append(random.randint(65536, 1048576))
    
    # Very large range (1M to 16M)
    for _ in range(100):
        sizes.append(random.randint(1048576, 16777216))
    
    # Add specific problematic sizes (many factors of 2)
    for base in [256, 512, 1024, 2048, 4096]:
        for mult in [3, 5, 7, 9, 15, 21, 31, 63]:
            sizes.append(base * mult)
    
    # Add Fibonacci numbers
    a, b = 1, 1
    while b < 10000000:
        if b > 255:
            sizes.append(b)
        a, b = b, a + b
    
    # Add perfect squares
    for i in range(16, 4096):
        sizes.append(i * i)
    
    # Add prime gaps (numbers between consecutive primes)
    for p in primes[:10]:
        sizes.append(p + 2)
        sizes.append(p + 4)
        sizes.append(p + 6)
    
    # Remove duplicates and sort
    sizes = sorted(list(set(sizes)))
    
    # Limit to reasonable testing time while maintaining diversity
    # Sample evenly across the range
    if len(sizes) > 5000:
        step = len(sizes) // 5000
        sizes = sizes[::step][:5000]
    
    return sizes

def collect_results(sizes, num_tests=10000):
    """Run tests for all sizes and collect results"""
    results = []
    
    print(f"Running tests for {len(sizes)} table sizes...")
    print("This may take a while...\n")
    
    for i, size in enumerate(sizes):
        print(f"[{i+1}/{len(sizes)}] Testing table_size={size}...", end='', flush=True)
        
        data = run_hash_test(size, num_tests)
        if data:
            data['is_prime'] = is_prime(size)
            results.append(data)
            print(f" ✓ (avalanche={data['avalanche_score']:.3f}, chi²={data['chi_square']:.3f})")
        else:
            print(" ✗ (failed)")
    
    return results

def analyze_results(results):
    """Analyze results and generate statistics"""
    df = pd.DataFrame(results)
    
    # Overall statistics
    stats = {
        'overall': {
            'count': len(df),
            'avalanche_mean': df['avalanche_score'].mean(),
            'avalanche_std': df['avalanche_score'].std(),
            'chi_square_mean': df['chi_square'].mean(),
            'chi_square_std': df['chi_square'].std(),
            'collision_ratio_mean': df['collision_ratio'].mean(),
            'collision_ratio_std': df['collision_ratio'].std(),
        }
    }
    
    # Prime vs Composite
    prime_df = df[df['is_prime']]
    composite_df = df[~df['is_prime']]
    
    if len(prime_df) > 0:
        stats['prime'] = {
            'count': len(prime_df),
            'avalanche_mean': prime_df['avalanche_score'].mean(),
            'chi_square_mean': prime_df['chi_square'].mean(),
            'collision_ratio_mean': prime_df['collision_ratio'].mean(),
        }
    
    if len(composite_df) > 0:
        stats['composite'] = {
            'count': len(composite_df),
            'avalanche_mean': composite_df['avalanche_score'].mean(),
            'chi_square_mean': composite_df['chi_square'].mean(),
            'collision_ratio_mean': composite_df['collision_ratio'].mean(),
        }
    
    # Quality metrics (how many meet ideal criteria)
    good_avalanche = ((df['avalanche_score'] >= 0.45) & (df['avalanche_score'] <= 0.55)).sum()
    good_chi = ((df['chi_square'] >= 0.9) & (df['chi_square'] <= 1.1)).sum()
    good_collision = (df['collision_ratio'] <= 1.2).sum()
    
    stats['quality'] = {
        'good_avalanche_pct': 100.0 * good_avalanche / len(df),
        'good_chi_square_pct': 100.0 * good_chi / len(df),
        'good_collision_pct': 100.0 * good_collision / len(df),
    }
    
    # Find best and worst performers
    stats['best'] = {
        'avalanche': df.loc[df['avalanche_score'].sub(0.5).abs().idxmin()],
        'chi_square': df.loc[df['chi_square'].sub(1.0).abs().idxmin()],
        'collision': df.loc[df['collision_ratio'].idxmin()],
    }
    
    stats['worst'] = {
        'avalanche': df.loc[df['avalanche_score'].sub(0.5).abs().idxmax()],
        'chi_square': df.loc[df['chi_square'].sub(1.0).abs().idxmax()],
        'collision': df.loc[df['collision_ratio'].idxmax()],
    }
    
    return stats, df

def generate_latex_tables(stats, df):
    """Generate LaTeX tables for the whitepaper"""
    latex_output = []
    
    # Overall statistics table
    latex_output.append(r"""
\begin{table}[h]
\centering
\caption{Overall GoldenHash Performance Statistics}
\begin{tabular}{lcc}
\hline
Metric & Mean & Std Dev \\
\hline
Avalanche Score & %.4f & %.4f \\
Chi-Square & %.4f & %.4f \\
Collision Ratio & %.4f & %.4f \\
\hline
\end{tabular}
\end{table}
""" % (stats['overall']['avalanche_mean'], stats['overall']['avalanche_std'],
       stats['overall']['chi_square_mean'], stats['overall']['chi_square_std'],
       stats['overall']['collision_ratio_mean'], stats['overall']['collision_ratio_std']))
    
    # Prime vs Composite comparison
    if 'prime' in stats and 'composite' in stats:
        latex_output.append(r"""
\begin{table}[h]
\centering
\caption{Performance Comparison: Prime vs Composite Table Sizes}
\begin{tabular}{lccc}
\hline
Table Type & Count & Avalanche & Chi-Square & Collision Ratio \\
\hline
Prime & %d & %.4f & %.4f & %.4f \\
Composite & %d & %.4f & %.4f & %.4f \\
\hline
\end{tabular}
\end{table}
""" % (stats['prime']['count'], stats['prime']['avalanche_mean'], 
       stats['prime']['chi_square_mean'], stats['prime']['collision_ratio_mean'],
       stats['composite']['count'], stats['composite']['avalanche_mean'],
       stats['composite']['chi_square_mean'], stats['composite']['collision_ratio_mean']))
    
    # Quality metrics
    latex_output.append(r"""
\begin{table}[h]
\centering
\caption{Quality Metrics: Percentage Meeting Ideal Criteria}
\begin{tabular}{lcc}
\hline
Metric & Ideal Range & Percentage Meeting \\
\hline
Avalanche Score & 0.45--0.55 & %.1f\%% \\
Chi-Square & 0.9--1.1 & %.1f\%% \\
Collision Ratio & $\leq$ 1.2 & %.1f\%% \\
\hline
\end{tabular}
\end{table}
""" % (stats['quality']['good_avalanche_pct'],
       stats['quality']['good_chi_square_pct'],
       stats['quality']['good_collision_pct']))
    
    # Sample results table (top 10)
    top_10 = df.nsmallest(10, 'table_size')[['table_size', 'is_prime', 'avalanche_score', 'chi_square', 'collision_ratio']]
    
    latex_output.append(r"""
\begin{table}[h]
\centering
\caption{Sample Results for Small Table Sizes}
\begin{tabular}{rcccc}
\hline
Table Size & Type & Avalanche & Chi-Square & Collision Ratio \\
\hline
""")
    
    for _, row in top_10.iterrows():
        type_str = "Prime" if row['is_prime'] else "Comp."
        latex_output.append("%d & %s & %.4f & %.4f & %.4f \\\\\n" % 
                          (row['table_size'], type_str, row['avalanche_score'], 
                           row['chi_square'], row['collision_ratio']))
    
    latex_output.append(r"""\hline
\end{tabular}
\end{table}
""")
    
    return ''.join(latex_output)

def save_results(results, stats, df, latex):
    """Save all results to files"""
    # Save raw JSON data
    with open('goldenhash_results.json', 'w') as f:
        json.dump(results, f, indent=2)
    print("\nRaw results saved to: goldenhash_results.json")
    
    # Save summary statistics
    with open('goldenhash_stats.json', 'w') as f:
        # Convert DataFrame objects to dict for JSON serialization
        stats_json = stats.copy()
        for key in ['best', 'worst']:
            if key in stats_json:
                for subkey in stats_json[key]:
                    if hasattr(stats_json[key][subkey], 'to_dict'):
                        stats_json[key][subkey] = stats_json[key][subkey].to_dict()
        json.dump(stats_json, f, indent=2)
    print("Statistics saved to: goldenhash_stats.json")
    
    # Save CSV for further analysis
    df.to_csv('goldenhash_results.csv', index=False)
    print("CSV data saved to: goldenhash_results.csv")
    
    # Save LaTeX tables
    with open('goldenhash_tables.tex', 'w') as f:
        f.write(latex)
    print("LaTeX tables saved to: goldenhash_tables.tex")
    
    # Save a summary report
    with open('goldenhash_report.txt', 'w') as f:
        f.write("GoldenHash Performance Analysis Report\n")
        f.write("=" * 50 + "\n\n")
        
        f.write(f"Total table sizes tested: {stats['overall']['count']}\n")
        if 'prime' in stats:
            f.write(f"Prime table sizes: {stats['prime']['count']}\n")
        if 'composite' in stats:
            f.write(f"Composite table sizes: {stats['composite']['count']}\n")
        f.write("\n")
        
        f.write("Overall Performance:\n")
        f.write(f"  Avalanche: {stats['overall']['avalanche_mean']:.4f} ± {stats['overall']['avalanche_std']:.4f}\n")
        f.write(f"  Chi-Square: {stats['overall']['chi_square_mean']:.4f} ± {stats['overall']['chi_square_std']:.4f}\n")
        f.write(f"  Collision Ratio: {stats['overall']['collision_ratio_mean']:.4f} ± {stats['overall']['collision_ratio_std']:.4f}\n")
        f.write("\n")
        
        f.write("Quality Metrics:\n")
        f.write(f"  Good avalanche (0.45-0.55): {stats['quality']['good_avalanche_pct']:.1f}%\n")
        f.write(f"  Good chi-square (0.9-1.1): {stats['quality']['good_chi_square_pct']:.1f}%\n")
        f.write(f"  Good collision ratio (≤1.2): {stats['quality']['good_collision_pct']:.1f}%\n")
        
    print("Summary report saved to: goldenhash_report.txt")

def main():
    """Main function"""
    print("GoldenHash Whitepaper Results Generator")
    print("=" * 40)
    
    # Generate test sizes
    sizes = generate_test_sizes()
    print(f"\nGenerated {len(sizes)} test sizes")
    print(f"Range: {min(sizes)} to {max(sizes)}")
    
    # Run tests (reduced per-size tests for large dataset)
    results = collect_results(sizes, num_tests=5000)
    
    if not results:
        print("\nError: No results collected!")
        return 1
    
    print(f"\nSuccessfully collected results for {len(results)} table sizes")
    
    # Analyze results
    stats, df = analyze_results(results)
    
    # Generate LaTeX tables
    latex = generate_latex_tables(stats, df)
    
    # Save everything
    save_results(results, stats, df, latex)
    
    print("\nDone! Results are ready for inclusion in the whitepaper.")
    print("\nTo include in LaTeX, use: \\input{goldenhash_tables.tex}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())