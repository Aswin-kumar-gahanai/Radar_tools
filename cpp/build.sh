#!/bin/bash

# Build script for C++ Radar System

set -e

echo "Building C++ Radar System..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build with all available cores
make -j$(nproc)

echo "Build complete! Executable: ./radar_system"
echo ""
echo "Usage examples:"
echo "  ./radar_system --live /dev/ttyACM0"
echo "  ./radar_system --replay ../data/recordings/radar_file.bin"
echo "  ./radar_system --help"