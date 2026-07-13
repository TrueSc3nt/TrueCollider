#!/bin/bash
# Build TrueCollider on Android/Termux aarch64.
# Run this script on the Termux device.

set -e

echo "TrueCollider - Termux aarch64 build"
echo "===================================="

if ! command -v cmake &> /dev/null; then
    echo "[+] CMake not found. Installing..."
    pkg update
    pkg install -y cmake make clang
fi

if [ ! -f "CMakeLists.txt" ]; then
    echo "[E] Run this script from the TrueCollider source root."
    exit 1
fi

rm -rf build-termux
cmake -B build-termux \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-termux -j$(nproc)

if [ -f build-termux/keyhunt ]; then
    echo ""
    echo "Build successful!"
    echo "Output: build-termux/keyhunt"
    file build-termux/keyhunt
else
    echo "[E] Build failed: build-termux/keyhunt not found"
    exit 1
fi
