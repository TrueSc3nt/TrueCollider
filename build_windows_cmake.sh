#!/bin/bash
# TrueCollider Windows .exe cross-compile using CMake + MinGW-w64.
# Produces a static 64-bit Windows executable from WSL/Linux.
#
# Run in WSL (Ubuntu/Debian) with:
#   bash build_windows_cmake.sh

set -e

echo "TrueCollider - Windows .exe CMake Cross-Build"
echo "=============================================="

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

echo "[+] Using cross-compiler:"
x86_64-w64-mingw32-g++ --version | head -n 1

rm -rf build-win
echo "[+] Configuring CMake with MinGW toolchain..."
cmake -B build-win \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
    -DSTATIC_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "[+] Building static Windows .exe..."
cmake --build build-win -j$(nproc)

if [ -f build-win/keyhunt.exe ]; then
    echo ""
    echo "Build successful!"
    echo "Output: build-win/keyhunt.exe"
    file build-win/keyhunt.exe || true
    ls -lh build-win/keyhunt.exe
else
    echo "[E] Build failed: build-win/keyhunt.exe not produced"
    exit 1
fi
