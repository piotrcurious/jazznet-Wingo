#!/bin/bash
set -e
echo "Running all EV-PWM Ensemble & Awareness Tests..."
./tests/runner
./tests/test_extended
./tests/test_stress
./tests/test_social
./tests/test_harmony
./tests/test_ensemble_dynamics
./tests/test_geo_awareness
./tests/test_concurrency
./tests/test_synchronization
./tests/test_role_dynamics
./tests/test_collective_intelligence
echo "All 11 test suites passed successfully!"
