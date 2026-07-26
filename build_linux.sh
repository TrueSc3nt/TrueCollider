#!/bin/bash
# TrueCollider generic Linux build script
# Works on x86_64 and ARM64
set -euo pipefail
cd "$(dirname "$0")"

echo "TrueCollider - Linux Build"
echo "=========================="

if [[ ! -f keyhunt.cpp ]]; then
    echo "[E] keyhunt.cpp missing - sources look incomplete/corrupt."
    echo "    Re-clone from https://github.com/TrueSc3nt/TrueCollider"
    exit 1
fi
if [[ ! -s Makefile ]]; then
    echo "[E] Makefile missing or empty/corrupt."
    exit 1
fi

ARCH=$(uname -m)
echo "Detected architecture: $ARCH"

if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i686" ] || [ "$ARCH" = "amd64" ]; then
    echo "Building for x86_64 with SSE..."
    make clean 2>/dev/null || true
    make -j"$(nproc 2>/dev/null || echo 4)"
elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ] || [ "$ARCH" = "armv7l" ]; then
    echo "Building for ARM (no SSE)..."
    make clean 2>/dev/null || true
    make -j"$(nproc 2>/dev/null || echo 4)" CXXFLAGS_BASE="-Wall -Wextra -O2" CFLAGS_BASE="-Wall -Wextra -O2"
else
    echo "Unknown architecture: $ARCH"
    echo "Trying generic build..."
    make clean 2>/dev/null || true
    make -j"$(nproc 2>/dev/null || echo 4)" CXXFLAGS_BASE="-Wall -Wextra -O2" CFLAGS_BASE="-Wall -Wextra -O2"
fi

if [[ ! -f keyhunt ]]; then
    echo "[E] Build failed: keyhunt was not produced"
    exit 1
fi
if [[ ! -s keyhunt ]]; then
    echo "[E] keyhunt is 0 bytes - corrupt output. Delete it and rebuild."
    rm -f keyhunt
    exit 1
fi

chmod +x keyhunt
echo ""
echo "Build successful! ($(wc -c < keyhunt) bytes)"
echo "Run: ./keyhunt -h"
echo "PASS: Linux CPU build"
