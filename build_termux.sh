#!/bin/bash
# Build TrueCollider on Android/Termux aarch64.
# Run this script on the Termux device.

set -euo pipefail
cd "$(dirname "$0")"

echo "TrueCollider - Termux aarch64 build"
echo "===================================="

if [[ ! -f keyhunt.cpp ]]; then
    echo "[E] keyhunt.cpp missing - sources look incomplete/corrupt."
    exit 1
fi
if [[ ! -f CMakeLists.txt ]]; then
    echo "[E] Run this script from the TrueCollider source root."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "[+] CMake not found. Installing..."
    pkg update
    pkg install -y cmake make clang
fi

rm -rf build-termux
cmake -B build-termux \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-termux -j"$(nproc 2>/dev/null || echo 4)"

OUT=""
if [[ -f build-termux/keyhunt ]]; then
    OUT=build-termux/keyhunt
elif [[ -f build-termux/keyhunt.exe ]]; then
    OUT=build-termux/keyhunt.exe
fi

if [[ -z "$OUT" ]]; then
    echo "[E] Build failed: keyhunt not found under build-termux/"
    exit 1
fi
if [[ ! -s "$OUT" ]]; then
    echo "[E] $OUT is 0 bytes - corrupt output. Delete build-termux and rebuild."
    rm -f "$OUT"
    exit 1
fi

echo ""
echo "Build successful! ($(wc -c < "$OUT") bytes)"
echo "Output: $OUT"
file "$OUT" || true
echo "PASS: Termux build"
