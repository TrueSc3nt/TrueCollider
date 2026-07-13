# Building TrueCollider

TrueCollider supports two build systems:

1. **Makefile** — traditional, works everywhere g++ and make are available.
2. **CMake** — recommended for cross-platform, IDE, and CI builds.

Both produce the same binary (`keyhunt` on Linux/macOS, `keyhunt.exe` on Windows).

---

## Linux (x86_64)

### Makefile
```bash
make -j$(nproc)
./keyhunt -h
```

### CMake
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/keyhunt -h
```

---

## Windows (cross-compile from WSL/Linux)

### Option A: Makefile script
```bash
bash build_windows.sh
# Output: keyhunt.exe
```

### Option B: CMake script
```bash
bash build_windows_cmake.sh
# Output: build-win/keyhunt.exe
```

### Option C: Manual CMake
```bash
sudo apt install mingw-w64 cmake
cmake -B build-win \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
    -DSTATIC_BUILD=ON
cmake --build build-win -j$(nproc)
```

---

## Termux / Android (aarch64)

Run on the Android device inside Termux:

```bash
pkg install cmake make clang
bash build_termux.sh
# Output: build-termux/keyhunt
```

Or manually:
```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake
cmake --build build -j$(nproc)
```

---

## macOS (x86_64 / Apple Silicon)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)
```

Note: Apple Silicon uses ARM64, so SSE is automatically disabled.

---

## Build options

| Option | Default | Description |
|--------|---------|-------------|
| `CPU_GRP_SIZE` | `1024` | Batch size for public-key generation |
| `ENABLE_SSE` | `ON` | Use SSE/SSSE3 on x86_64 |
| `STATIC_BUILD` | `OFF` | Fully static executable (Linux/MinGW) |
| `BUILD_TESTS` | `ON` | Build `test_binaryfuse` |
| `ENABLE_LTO` | `OFF` | Link-time optimization |
| `ENABLE_CUDA` | `OFF` | CUDA backend scaffolding |
| `ENABLE_OPENCL` | `OFF` | OpenCL backend scaffolding |

Examples:

```bash
# Larger batch size for high-end CPUs
make CPU_GRP_SIZE=4096 -j$(nproc)

# CMake with larger batch size and LTO
cmake -B build -DCPU_GRP_SIZE=4096 -DENABLE_LTO=ON
cmake --build build -j$(nproc)

# Disable SSE for a generic portable build
cmake -B build -DENABLE_SSE=OFF
cmake --build build -j$(nproc)
```

---

## IDE support

The CMake project is IDE-friendly. For CLion or Visual Studio with CMake:

1. Open the project root (where `CMakeLists.txt` lives).
2. Select the `keyhunt` target.
3. Build.

For Windows/Visual Studio native builds, the project currently requires MinGW or WSL because it uses POSIX headers (`getopt.h`, `pthread.h`). Native MSVC support is a future TODO.

---

## Troubleshooting

### `pthread.h` not found on Windows
Make sure you are using MinGW-w64, not MSVC. Install `mingw-w64` in WSL or MSYS2.

### `-static` fails with missing libwinpthread.a
On some distributions the static winpthread library is in a separate package:
```bash
sudo apt install mingw-w64-winpthreads-static   # Debian/Ubuntu
sudo dnf install mingw64-winpthreads-static       # Fedora
```

### Termux build fails with SSE errors
Use the Termux toolchain file, which disables SSE:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake
```
