#!/bin/bash
# TrueCollider generic Linux build script
# Works on x86_64 and ARM64

echo "TrueCollider - Linux Build"
echo "=========================="

ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i686" ] || [ "$ARCH" = "amd64" ]; then
    echo "Building for x86_64 with SSE..."
    make clean 2>/dev/null
    make
elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ] || [ "$ARCH" = "armv7l" ]; then
    echo "Building for ARM (no SSE)..."
    make clean 2>/dev/null
    make CXXFLAGS_BASE="-Wall -Wextra -O2" CFLAGS_BASE="-Wall -Wextra -O2"
else
    echo "Unknown architecture: $ARCH"
    echo "Trying generic build..."
    make clean 2>/dev/null
    make CXXFLAGS_BASE="-Wall -Wextra -O2" CFLAGS_BASE="-Wall -Wextra -O2"
fi

if [ -f keyhunt ]; then
    chmod +x keyhunt
    echo ""
    echo "Build successful!"
    echo "Run: ./keyhunt -h"
else
    echo "Build failed!"
    exit 1
fi
