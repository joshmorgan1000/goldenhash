#!/bin/bash
# Overnight CROCS test suite
# Focuses on statistical significance with appropriate test counts

echo "CROCS Overnight Test Suite"
echo "=========================="
echo "Start time: $(date)"
echo ""

# Create results directory
mkdir -p overnight_results

# Test 1: Small primes with heavy testing
echo "Phase 1: Small primes (97-997) with 10M tests each"
for size in 97 197 397 797 997; do
    echo -n "Testing size $size with 10M tests... "
    time ./build/test_comprehensive --size=$size --tests=10000000 --csv-output >> overnight_results/phase1.csv
done

# Test 2: Powers of 2 region with appropriate tests
echo -e "\nPhase 2: Powers of 2 (±10) with scaled tests"
for base in 8 10 12 14 16 18 20; do
    size=$((2**base))
    tests=$((size * 1000))  # 1000x table size
    echo -n "Testing size $size with ${tests} tests... "
    time ./build/test_comprehensive --size=$size --tests=$tests --csv-output >> overnight_results/phase2.csv
done

# Test 3: Large primes with massive tests
echo -e "\nPhase 3: Large primes with 100M+ tests"
for size in 10007 100003 1000003; do
    tests=$((size * 100))  # 100x table size
    echo -n "Testing size $size with ${tests} tests... "
    time ./build/test_comprehensive --size=$size --tests=$tests --csv-output >> overnight_results/phase3.csv
done

# Test 4: Golden ratio special sizes
echo -e "\nPhase 4: Golden ratio related sizes"
for n in 1000 10000 100000 1000000; do
    size=$(python3 -c "import math; print(int($n / 1.618))")
    tests=$((n * 10))
    echo -n "Testing size $size (n=$n) with ${tests} tests... "
    time ./build/test_comprehensive --size=$size --tests=$tests --csv-output >> overnight_results/phase4.csv
done

echo -e "\nEnd time: $(date)"
echo "Results saved in overnight_results/"

# Import to database
echo -e "\nImporting results to database..."
python3 - << EOF
from data_collector import CROCSDataCollector
import os

collector = CROCSDataCollector()
run_id = collector.create_test_run("overnight", notes="Overnight statistical test")

for phase in ['phase1.csv', 'phase2.csv', 'phase3.csv', 'phase4.csv']:
    path = f'overnight_results/{phase}'
    if os.path.exists(path):
        print(f"Importing {phase}...")
        collector.import_csv_data(path, run_id)

collector.close()
print("Database import complete!")
EOF