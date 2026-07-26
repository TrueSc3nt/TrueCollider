#!/bin/bash
# TrueCollider Windows .exe cross-compile script for WSL/Linux
# Produces a 64-bit Windows executable using MinGW-w64.
#
# Run in WSL (Ubuntu/Debian) with:
#   bash build_windows.sh

set -euo pipefail
cd "$(dirname "$0")"

echo "TrueCollider - Windows .exe Cross-Build"
echo "========================================="

if [[ ! -f keyhunt.cpp ]]; then
    echo "[E] keyhunt.cpp missing - sources look incomplete/corrupt."
    echo "    Re-clone from https://github.com/TrueSc3nt/TrueCollider"
    exit 1
fi
if [[ ! -s Makefile ]]; then
    echo "[E] Makefile missing or empty/corrupt."
    exit 1
fi

# Check for MinGW cross-compiler
if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "[+] MinGW-w64 not found. Installing..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y mingw-w64
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y mingw64-gcc-c++ mingw64-winpthreads-static
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --needed mingw-w64-gcc
    else
        echo "[E] No supported package manager found. Please install MinGW-w64 manually."
        exit 1
    fi
fi

echo "[+] Using cross-compiler:"
x86_64-w64-mingw32-g++ --version | head -n 1

echo "[+] Building static Windows .exe..."
make clean
make windows

if [[ ! -f keyhunt.exe ]]; then
    echo "[E] Build failed: keyhunt.exe not produced"
    exit 1
fi
if [[ ! -s keyhunt.exe ]]; then
    echo "[E] keyhunt.exe is 0 bytes - corrupt output. Delete it and rebuild."
    rm -f keyhunt.exe
    exit 1
fi

echo ""
echo "Build successful! ($(wc -c < keyhunt.exe) bytes)"
echo "Output: keyhunt.exe"
file keyhunt.exe || true
ls -lh keyhunt.exe
echo "PASS: Windows cross-build"
