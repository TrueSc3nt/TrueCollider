#!/bin/bash
# TrueCollider Termux build script
# Run this on Android/Termux: bash build_termux.sh

echo "TrueCollider - Termux/ARM Build"
echo "================================"

# Install dependencies if needed
if ! command -v g++ &> /dev/null; then
    echo "Installing build tools..."
    pkg install -y clang make
fi

# Termux uses clang, not g++
export CC=clang
export CXX=clang++

# Termux is ARM — disable SSE
echo "Building for ARM (no SSE)..."
make clean 2>/dev/null
make CC=clang CXX=clang++ CXXFLAGS_BASE="-Wall -Wextra -O2" CFLAGS_BASE="-Wall -Wextra -O2"

if [ -f keyhunt ]; then
    chmod +x keyhunt
    echo ""
    echo "Build successful!"
    echo "Run: ./keyhunt -h"
else
    echo "Build failed!"
    exit 1
fi
