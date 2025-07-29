#!/bin/bash

# Run all CROCS comprehensive tests
# This will take several hours

echo 'Starting comprehensive CROCS testing...'
echo 'Total configurations: 10'
echo 'Total test cases: 3512'

echo '\n=== Running quick_smoke ==='
echo 'Quick smoke test with representative sizes'
echo 'Sizes: 12'
python3 run_config_tests.py config_quick_smoke.json
sleep 2

echo '\n=== Running comprehensive_small ==='
echo 'All sizes under 5K'
echo 'Sizes: 229'
python3 run_config_tests.py config_comprehensive_small.json
sleep 2

echo '\n=== Running powers_of_2 ==='
echo 'Powers of 2 and nearby values'
echo 'Sizes: 18'
python3 run_config_tests.py config_powers_of_2.json
sleep 2

echo '\n=== Running mathematical ==='
echo 'Mersenne, Fibonacci, and powers of 2'
echo 'Sizes: 44'
python3 run_config_tests.py config_mathematical.json
sleep 2

echo '\n=== Running stress_composite ==='
echo 'Highly composite numbers (many factors)'
echo 'Sizes: 98'
python3 run_config_tests.py config_stress_composite.json
sleep 2

echo '\n=== Running comprehensive_medium ==='
echo 'All medium sizes (5K-50K)'
echo 'Sizes: 96'
python3 run_config_tests.py config_comprehensive_medium.json
sleep 2

echo '\n=== Running large_scale ==='
echo 'Large sizes (50K-500K)'
echo 'Sizes: 189'
python3 run_config_tests.py config_large_scale.json
sleep 2

echo '\n=== Running extreme_scale ==='
echo 'Very large sizes (500K+)'
echo 'Sizes: 60'
python3 run_config_tests.py config_extreme_scale.json
sleep 2

echo '\n=== Running prime_density ==='
echo 'Prime density at different scales'
echo 'Sizes: 540'
python3 run_config_tests.py config_prime_density.json
sleep 2

echo '\n=== Running golden_ratio_focus ==='
echo 'Sizes related to golden ratio'
echo 'Sizes: 2226'
python3 run_config_tests.py config_golden_ratio_focus.json
sleep 2

echo '\nAll tests complete!'
python3 data_collector.py analyze
