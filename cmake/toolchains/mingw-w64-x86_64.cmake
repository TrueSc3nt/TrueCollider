# Toolchain file for cross-compiling TrueCollider for Windows x86_64 from WSL/Linux.
#
# Usage:
#   sudo apt install mingw-w64
#   cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
#         -DSTATIC_BUILD=ON
#   cmake --build build-win -j$(nproc)
#
# Produces: build-win/keyhunt.exe

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Disable SSE on the cross build by default to avoid MinGW intrinsics issues.
# You can override with -DENABLE_SSE=ON if you are sure your toolchain supports it.
set(ENABLE_SSE OFF CACHE BOOL "Disable SSE for MinGW cross-build by default")
set(STATIC_BUILD ON CACHE BOOL "Static Windows executable by default")
