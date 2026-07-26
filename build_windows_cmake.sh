#!/bin/bash
# TrueCollider Windows .exe cross-compile using CMake + MinGW-w64.
# Produces a static 64-bit Windows executable from WSL/Linux.
#
# Run in WSL (Ubuntu/Debian) with:
#   bash build_windows_cmake.sh

set -euo pipefail
cd "$(dirname "$0")"

echo "TrueCollider - Windows .exe CMake Cross-Build"
echo "=============================================="

if [[ ! -f keyhunt.cpp ]]; then
    echo "[E] keyhunt.cpp missing - sources look incomplete/corrupt."
    echo "    Re-clone from https://github.com/TrueSc3nt/TrueCollider"
    exit 1
fi
if [[ ! -f CMakeLists.txt ]]; then
    echo "[E] CMakeLists.txt missing - run from TrueCollider source root."
    exit 1
fi

if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "[+] MinGW-w64 not found. Installing..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y mingw-w64 cmake
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y mingw64-gcc-c++ mingw64-winpthreads-static cmake
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --needed mingw-w64-gcc cmake
    else
        echo "[E] No supported package manager found. Please install MinGW-w64 and cmake."
        exit 1
    fi
fi

if ! command -v cmake &> /dev/null; then
    echo "[E] cmake not found. Install cmake and re-run."
    exit 1
fi

echo "[+] Using cross-compiler:"
x86_64-w64-mingw32-g++ --version | head -n 1

rm -rf build-win
echo "[+] Configuring CMake with MinGW toolchain..."
cmake -B build-win \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
    -DSTATIC_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "[+] Building static Windows .exe..."
cmake --build build-win -j"$(nproc 2>/dev/null || echo 4)"

OUT=""
if [[ -f build-win/keyhunt.exe ]]; then
    OUT=build-win/keyhunt.exe
elif [[ -f build-win/Release/keyhunt.exe ]]; then
    OUT=build-win/Release/keyhunt.exe
fi

if [[ -z "$OUT" ]]; then
    echo "[E] Build failed: keyhunt.exe not produced under build-win/"
    exit 1
fi
if [[ ! -s "$OUT" ]]; then
    echo "[E] $OUT is 0 bytes - corrupt output. Delete build-win and rebuild."
    rm -f "$OUT"
    exit 1
fi

echo ""
echo "Build successful! ($(wc -c < "$OUT") bytes)"
echo "Output: $OUT"
file "$OUT" || true
ls -lh "$OUT"
echo "PASS: Windows CMake cross-build"
