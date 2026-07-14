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
| `ENABLE_AVX` | `ON` | Enable AVX/AVX2 runtime detection |
| `ENABLE_AVX512` | `ON` | Enable AVX-512 runtime detection |
| `STATIC_BUILD` | `OFF` | Fully static executable (Linux/MinGW) |
| `BUILD_TESTS` | `ON` | Build `test_binaryfuse` |
| `ENABLE_LTO` | `OFF` | Link-time optimization |
| `ENABLE_CUDA` | `OFF` | CUDA backend (requires CUDA toolkit) |
| `ENABLE_OPENCL` | `OFF` | OpenCL backend (requires OpenCL headers/runtime) |

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

For Windows/Visual Studio native x64 builds, CMake vendors `compat/getopt`, links only `ws2_32`, and sets `/arch:AVX512` only on the AVX-512 hash source file. MinGW cross-builds from WSL/Linux produce the same runtime behavior.

### Native MSVC (Visual Studio)

```powershell
cmake -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
```

Or open the folder in Visual Studio (CMake integration) and build the `keyhunt` target.

### Windows — any CPU

The Windows `.exe` (MinGW or MSVC x64) is safe to run on **any** 64-bit x86 PC:

| CPU | What happens |
|-----|----------------|
| AVX-512F + BW (OS enabled) | `-A auto` or `-A avx512` uses the 16-wide kernel after self-test |
| AVX2 / AVX / SSE only | Auto-clamps to the best supported level; search uses SSE 4-wide batches |
| Very old / no SSE2 | Falls back to scalar hash paths |
| ARM64 Windows | SSE/AVX-512 disabled at build time; portable scalar build |

Startup never requires AVX-512. If `-A avx512` is requested on unsupported hardware, or the self-test fails, the tool prints a warning and continues on SSE.

With `-e` (endomorphism) and compress-only search, AVX-512 also accelerates the λ/λ² variants. Uncompressed+endo stays on the 4-wide SSE path.

---

## Native Windows CPU (MinGW)

```bat
build_mingw_native.bat
REM Output: keyhunt.exe
```

Auto-detects MSYS2 `mingw64` / `ucrt64` / `clang64` (or `MINGW_HOME`). Falls back to `g++` on `PATH`.

## Native Windows + CUDA (NVIDIA)

Requires **VS 2022 Build Tools** (preferred for `nvcc` host compat) and a **complete** CUDA 12.x toolkit (`nvcc.exe` **and** `include\cuda_runtime.h`). Partial installs are skipped.

```bat
build_cuda_vs2022.bat
REM Output: keyhunt_cuda.exe  (also build-cuda-vs2022\keyhunt.exe)
REM Alternate: build_cuda_msvc.bat
```

Scripts auto-detect VS via `vswhere`, prefer VS 2022, then fall back to newer VS with `-allow-unsupported-compiler`.

Manual:

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8
cmake -B build-cuda-vs2022 -G Ninja -DENABLE_CUDA=ON -DENABLE_AVX512=ON
cmake --build build-cuda-vs2022 -j
```

Run:

```bat
keyhunt_cuda.exe -m address -f targets.txt -U cuda -t 1 -l compress -s 5
```

---

## OpenCL (AMD / NVIDIA / Intel)

```bash
cmake -B build-opencl -DENABLE_OPENCL=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-opencl -j
./build-opencl/keyhunt -m address -f targets.txt -U opencl -t 8 -l compress
```

Install the vendor OpenCL ICD (AMD Adrenalin, NVIDIA GPU driver OpenCL, or Intel). Headers must provide `CL/cl.h` (or `OpenCL/cl.h` on macOS).

---

## Troubleshooting

### `pthread.h` not found on Windows
- **MSVC:** use the CMake project above (Win32 threads + vendored getopt; no pthread needed).
- **MinGW:** install `mingw-w64` in WSL or MSYS2.

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

### CUDA build: `nvcc` cannot find MSVC
Use **VS 2022** `vcvars64.bat`, not only VS 2025/18. See `build_cuda_vs2022.bat`.

### CUDA: `cuda_runtime.h` not found
The selected toolkit is incomplete. Reinstall CUDA 12.x, or delete partial version folders missing `include\cuda_runtime.h`. The build bats skip those automatically.

### OpenCL: no devices
Install a GPU OpenCL driver ICD. `topencl_hello` lists platforms/devices at startup with `-U opencl`.
