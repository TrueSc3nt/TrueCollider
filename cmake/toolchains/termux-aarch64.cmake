# Toolchain file for building TrueCollider on Android/Termux aarch64.
#
# Usage (on device in Termux):
#   pkg install cmake make clang
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake
#   cmake --build build -j$(nproc)
#
# Produces: build/keyhunt

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)

set(TERMUX ON CACHE BOOL "Building for Termux/Android")
set(ENABLE_SSE OFF CACHE BOOL "Termux is ARM - SSE disabled")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
