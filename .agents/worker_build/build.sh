#!/bin/bash
# Exit on any error
set -e

echo "=== STEP 1: Creating build directory ==="
mkdir -p /Users/jacko/Documents/myEmulationStation/build

echo "=== STEP 2: Running CMake configuration ==="
cd /Users/jacko/Documents/myEmulationStation/build
cmake ..

echo "=== STEP 3: Compiling the project ==="
make -j$(sysctl -n hw.ncpu || nproc)

echo "=== STEP 4: Deploying binary to es_test ==="
mkdir -p /Users/jacko/Documents/myEmulationStation/es_test
cp emulationstation /Users/jacko/Documents/myEmulationStation/es_test/

echo "=== BUILD AND DEPLOY COMPLETED SUCCESSFULLY ==="
