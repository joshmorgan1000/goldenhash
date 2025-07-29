#!/usr/bin/env python3
"""
CROCS Data Analysis and Visualization
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
from scipy import stats
import subprocess
import json
import os

# Set style
plt.style.use('seaborn-v0_8-darkgrid')
sns.set_palette("husl")

def run_cpp_tests():
    """Run C++ tests and capture output"""
    print("Running comprehensive tests...")
    
    # Make sure we're in the build directory
    os.chdir('build')
    
    # Run tests and capture output
    result = subprocess.run(['./test_crocs_comprehensive'], 
                          capture_output=True, text=True)
    
    # The test should have created crocs_test_results.csv
    if os.path.exists('crocs_test_results.csv'):
        print("Test data collected successfully")
    else:
        print("Warning: CSV file not found")
        
    os.chdir('..')
    return result.stdout

def load_and_analyze_data():
    """Load CSV data and perform analysis"""
    df = pd.read_csv('build/crocs_test_results.csv')
    
    # Add some calculated columns
    df['log_table_size'] = np.log10(df['table_size'])
    df['bits_to_size_ratio'] = df['bits_needed'] / np.log2(df['table_size'])
    df['collision_error'] = np.abs(df['collision_ratio'] - 1.0)
    
    return df

def plot_chi_square_analysis(df):
    """Plot chi-square distribution quality"""
    plt.figure(figsize=(12, 6))
    
    plt.subplot(1, 2, 1)
    plt.scatter(df['log_table_size'], df['chi_square'], alpha=0.7)
    plt.axhline(y=1.0, color='r', linestyle='--', label='Ideal (1.0)')
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('Chi-Square (normalized)')
    plt.title('Chi-Square Distribution vs Table Size')
    plt.legend()
    
    plt.subplot(1, 2, 2)
    plt.hist(df['chi_square'], bins=20, edgecolor='black')
    plt.axvline(x=1.0, color='r', linestyle='--', label='Ideal')
    plt.xlabel('Chi-Square Value')
    plt.ylabel('Frequency')
    plt.title('Chi-Square Distribution')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('crocs/docs/chi_square_analysis.png', dpi=300)
    plt.close()

def plot_performance_scaling(df):
    """Plot performance characteristics"""
    plt.figure(figsize=(12, 6))
    
    plt.subplot(1, 2, 1)
    plt.scatter(df['log_table_size'], df['ns_per_hash'], alpha=0.7)
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('Nanoseconds per Hash')
    plt.title('Hash Performance vs Table Size')
    
    # Add trend line
    z = np.polyfit(df['log_table_size'], df['ns_per_hash'], 1)
    p = np.poly1d(z)
    plt.plot(df['log_table_size'], p(df['log_table_size']), "r--", alpha=0.8)
    
    plt.subplot(1, 2, 2)
    # Group by powers of 2 vs primes
    df['is_power_of_2'] = df['table_size'].apply(lambda x: (x & (x-1)) == 0)
    
    for is_pow2, group in df.groupby('is_power_of_2'):
        label = 'Powers of 2' if is_pow2 else 'Prime sizes'
        plt.scatter(group['log_table_size'], group['ns_per_hash'], 
                   label=label, alpha=0.7)
    
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('Nanoseconds per Hash')
    plt.title('Performance: Powers of 2 vs Primes')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('crocs/docs/performance_scaling.png', dpi=300)
    plt.close()

def plot_collision_analysis(df):
    """Plot collision rate analysis"""
    plt.figure(figsize=(12, 6))
    
    plt.subplot(1, 2, 1)
    plt.scatter(df['log_table_size'], df['collision_ratio'], alpha=0.7)
    plt.axhline(y=1.0, color='r', linestyle='--', label='Expected')
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('Actual/Expected Collisions')
    plt.title('Collision Ratio vs Table Size')
    plt.legend()
    
    plt.subplot(1, 2, 2)
    # Plot collision rate error distribution
    plt.hist(df['collision_error'], bins=20, edgecolor='black')
    plt.xlabel('Absolute Error from Expected')
    plt.ylabel('Frequency')
    plt.title('Collision Prediction Error Distribution')
    
    # Add statistics
    mean_error = df['collision_error'].mean()
    plt.axvline(x=mean_error, color='r', linestyle='--', 
                label=f'Mean: {mean_error:.4f}')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('crocs/docs/collision_analysis.png', dpi=300)
    plt.close()

def plot_golden_ratio_analysis(df):
    """Analyze golden ratio prime selection"""
    plt.figure(figsize=(12, 6))
    
    plt.subplot(1, 2, 1)
    # Calculate actual distance from golden ratio
    df['golden_ratio_actual'] = df['table_size'] / df['golden_prime']
    phi = 1.6180339887498948482
    
    plt.scatter(df['log_table_size'], df['golden_ratio_actual'], alpha=0.7)
    plt.axhline(y=phi, color='r', linestyle='--', label=f'φ = {phi:.6f}')
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('N / Prime')
    plt.title('Golden Ratio Accuracy')
    plt.legend()
    
    plt.subplot(1, 2, 2)
    # Prime distance distribution
    plt.scatter(df['log_table_size'], 
                df['prime_distance_from_golden'].abs(), 
                alpha=0.7)
    plt.xlabel('Log10(Table Size)')
    plt.ylabel('|Prime - Golden Value|')
    plt.title('Prime Selection Distance')
    plt.yscale('log')
    
    plt.tight_layout()
    plt.savefig('crocs/docs/golden_ratio_analysis.png', dpi=300)
    plt.close()

def plot_avalanche_analysis(df):
    """Plot avalanche effect analysis"""
    # Note: Current avalanche scores are problematic, but let's visualize anyway
    plt.figure(figsize=(10, 6))
    
    plt.scatter(df['bits_needed'], df['avalanche_score'], alpha=0.7)
    plt.axhline(y=0.5, color='r', linestyle='--', label='Ideal (0.5)')
    plt.xlabel('Bits Needed for Table Size')
    plt.ylabel('Avalanche Score')
    plt.title('Avalanche Effect vs Hash Size\n(Note: Test methodology needs fixing)')
    plt.legend()
    
    # Add trend line
    z = np.polyfit(df['bits_needed'], df['avalanche_score'], 1)
    p = np.poly1d(z)
    plt.plot(df['bits_needed'], p(df['bits_needed']), "g--", alpha=0.8, 
             label=f'Trend: {z[0]:.4f}x + {z[1]:.4f}')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig('crocs/docs/avalanche_analysis.png', dpi=300)
    plt.close()

def generate_summary_stats(df):
    """Generate summary statistics"""
    stats_dict = {
        'Total table sizes tested': len(df),
        'Table size range': f"{df['table_size'].min()} - {df['table_size'].max()}",
        'Chi-square mean': f"{df['chi_square'].mean():.4f} (ideal: 1.0)",
        'Chi-square std': f"{df['chi_square'].std():.4f}",
        'Performance mean': f"{df['ns_per_hash'].mean():.2f} ns",
        'Performance range': f"{df['ns_per_hash'].min():.2f} - {df['ns_per_hash'].max():.2f} ns",
        'Collision ratio mean': f"{df['collision_ratio'].mean():.4f} (ideal: 1.0)",
        'Collision ratio std': f"{df['collision_ratio'].std():.4f}",
    }
    
    # Write to file
    with open('crocs/docs/summary_statistics.json', 'w') as f:
        json.dump(stats_dict, f, indent=2)
    
    # Also create a markdown summary
    with open('crocs/docs/summary_statistics.md', 'w') as f:
        f.write("# CROCS Summary Statistics\n\n")
        for key, value in stats_dict.items():
            f.write(f"- **{key}**: {value}\n")
        
        f.write("\n## Key Findings\n\n")
        
        # Chi-square analysis
        chi_within_5pct = (df['chi_square'] >= 0.95) & (df['chi_square'] <= 1.05)
        f.write(f"- {chi_within_5pct.sum()} out of {len(df)} "
                f"({100*chi_within_5pct.sum()/len(df):.1f}%) table sizes "
                f"have chi-square within 5% of ideal\n")
        
        # Performance scaling
        correlation = df['log_table_size'].corr(df['ns_per_hash'])
        f.write(f"- Performance correlation with log(table_size): {correlation:.4f} "
                f"(near 0 indicates O(1) behavior)\n")
        
        # Collision accuracy
        collision_within_5pct = df['collision_error'] <= 0.05
        f.write(f"- {collision_within_5pct.sum()} out of {len(df)} "
                f"({100*collision_within_5pct.sum()/len(df):.1f}%) table sizes "
                f"have collision rates within 5% of expected\n")

def main():
    """Main analysis pipeline"""
    print("CROCS Data Analysis")
    print("==================")
    
    # Check if we need to run tests
    if not os.path.exists('build/crocs_test_results.csv'):
        print("No existing data found. Running tests...")
        run_cpp_tests()
    
    # Load and analyze data
    print("\nLoading data...")
    df = load_and_analyze_data()
    print(f"Loaded {len(df)} test results")
    
    # Generate plots
    print("\nGenerating visualizations...")
    os.makedirs('crocs/docs', exist_ok=True)
    
    plot_chi_square_analysis(df)
    print("  ✓ Chi-square analysis")
    
    plot_performance_scaling(df)
    print("  ✓ Performance scaling")
    
    plot_collision_analysis(df)
    print("  ✓ Collision analysis")
    
    plot_golden_ratio_analysis(df)
    print("  ✓ Golden ratio analysis")
    
    plot_avalanche_analysis(df)
    print("  ✓ Avalanche analysis")
    
    # Generate summary statistics
    generate_summary_stats(df)
    print("  ✓ Summary statistics")
    
    print("\nAnalysis complete! Check crocs/docs/ for results.")
    
    # Display summary
    print("\nQuick Summary:")
    print(f"  Chi-square: {df['chi_square'].mean():.4f} ± {df['chi_square'].std():.4f}")
    print(f"  Performance: {df['ns_per_hash'].mean():.1f} ± {df['ns_per_hash'].std():.1f} ns")
    print(f"  Collision accuracy: {(df['collision_error'] <= 0.05).sum()}/{len(df)} within 5%")

if __name__ == "__main__":
    main()