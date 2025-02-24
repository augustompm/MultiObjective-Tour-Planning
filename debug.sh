#!/bin/bash
# debug.sh - Automated GDB debugging for MultiObjective-Tour-Planning

# Set error handling
set -e

# Step 1: Clean and prepare build directory with debug symbols
echo "Preparing debug build..."
rm -rf build
mkdir -p build
cd build

# Step 2: Configure with debug symbols
echo "Configuring CMake with debug symbols..."
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Step 3: Build the project
echo "Building project with debug symbols..."
make

# Step 4: Create GDB command file
echo "Creating GDB command file..."
cat > gdb_commands.txt << EOF
run
bt
frame 0
print *this
info locals
up
quit
EOF

# Step 5: Run with GDB and save output
echo "Running program with GDB..."
gdb -q -x gdb_commands.txt ./bin/tourist_route > debug_output.txt 2>&1

# Step 6: Display relevant output
echo "Debug completed. Results saved to build/debug_output.txt"
echo "==================== DEBUG OUTPUT ===================="
grep -A 20 "Program received signal" debug_output.txt
echo "===================================================="

# Return to original directory
cd ..

echo "Debugging complete. Full output is in build/debug_output.txt"