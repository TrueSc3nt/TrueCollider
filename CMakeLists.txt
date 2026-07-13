cmake_minimum_required(VERSION 3.16)
project(TrueCollider VERSION 1.0.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(ENABLE_SSE          "Use SSE/SSSE3 optimizations on x86_64 (Linux/Windows)" ON)
option(ENABLE_AVX          "Enable AVX/AVX2 runtime detection (compile-time placeholder)" ON)
option(ENABLE_AVX512       "Enable AVX-512 runtime detection (compile-time placeholder)" ON)
option(ENABLE_CUDA         "Build CUDA GPU backend (experimental)" OFF)
option(ENABLE_OPENCL       "Build OpenCL GPU backend (experimental)" OFF)
option(STATIC_BUILD        "Build fully static executable where possible" OFF)
option(BUILD_TESTS         "Build small verification tests" ON)
option(ENABLE_LTO          "Enable link-time optimization" OFF)
set(CPU_GRP_SIZE "1024" CACHE STRING "Number of keys computed per CPU batch/group")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Platform / architecture detection
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(IS_WINDOWS TRUE)
else()
    set(IS_WINDOWS FALSE)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|i686|i386|X86")
    set(IS_X86 TRUE)
else()
    set(IS_X86 FALSE)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(IS_ARM64 TRUE)
else()
    set(IS_ARM64 FALSE)
endif()

# Termux is detected at runtime via CMake cache or env, not here.
# Build scripts should pass -DTERMUX=ON when required.
if(TERMUX)
    set(IS_TERMUX TRUE)
else()
    set(IS_TERMUX FALSE)
endif()

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
if(MSVC)
    # Only apply MSVC flags to C/C++ — bare flags break nvcc under Ninja.
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/W3>
        $<$<COMPILE_LANGUAGE:CXX>:/W3>
        $<$<COMPILE_LANGUAGE:C>:/O2>
        $<$<COMPILE_LANGUAGE:CXX>:/O2>
    )
    if(ENABLE_LTO)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/GL>)
        add_link_options(/LTCG)
    endif()
    if(IS_ARM64 OR NOT IS_X86)
        add_compile_definitions(NO_SSE)
    endif()
else()
    # GCC / Clang / MinGW
    add_compile_options(-Wall -Wextra -Wno-deprecated-copy -O2)

    if(ENABLE_LTO)
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()

    if(IS_X86 AND ENABLE_SSE AND NOT IS_TERMUX)
        add_compile_options(-m64 -mssse3 -ftree-vectorize)
        add_compile_definitions(HAVE_SSE)
    else()
        add_compile_definitions(NO_SSE)
    endif()

    if(IS_ARM64)
        add_compile_definitions(NO_SSE)
    endif()

endif()

if(STATIC_BUILD AND NOT MSVC)
    add_link_options(-static)
endif()

add_compile_definitions(CPU_GRP_SIZE=${CPU_GRP_SIZE})

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/base58
    ${CMAKE_SOURCE_DIR}/rmd160
    ${CMAKE_SOURCE_DIR}/oldbloom
    ${CMAKE_SOURCE_DIR}/bloom
    ${CMAKE_SOURCE_DIR}/binaryfuse
    ${CMAKE_SOURCE_DIR}/sha3
    ${CMAKE_SOURCE_DIR}/secp256k1
    ${CMAKE_SOURCE_DIR}/hash
    ${CMAKE_SOURCE_DIR}/xxhash
    ${CMAKE_SOURCE_DIR}/gpu
    ${CMAKE_SOURCE_DIR}/ed25519
)

# ---------------------------------------------------------------------------
# Core source files (same as the Makefile)
# ---------------------------------------------------------------------------
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/keyhunt.cpp
    ${CMAKE_SOURCE_DIR}/backend_config.cpp
    ${CMAKE_SOURCE_DIR}/cpu_features.cpp
    ${CMAKE_SOURCE_DIR}/gpu/gpu_dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/gpu/cuda/host_hash160_bridge.cpp
    ${CMAKE_SOURCE_DIR}/util.cpp
    ${CMAKE_SOURCE_DIR}/base58/base58.c
    ${CMAKE_SOURCE_DIR}/rmd160/rmd160.c
    ${CMAKE_SOURCE_DIR}/sha3/sha3.c
    ${CMAKE_SOURCE_DIR}/sha3/keccak.c
    ${CMAKE_SOURCE_DIR}/xxhash/xxhash.c
    ${CMAKE_SOURCE_DIR}/bloom/bloom.cpp
    ${CMAKE_SOURCE_DIR}/oldbloom/bloom.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/Int.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/Point.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/SECP256K1.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/IntMod.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/Random.cpp
    ${CMAKE_SOURCE_DIR}/secp256k1/IntGroup.cpp
    ${CMAKE_SOURCE_DIR}/hash/ripemd160.cpp
    ${CMAKE_SOURCE_DIR}/hash/sha256.cpp
    ${CMAKE_SOURCE_DIR}/hash/sha512.cpp
    ${CMAKE_SOURCE_DIR}/ed25519/fe.c
    ${CMAKE_SOURCE_DIR}/ed25519/ge.c
    ${CMAKE_SOURCE_DIR}/ed25519/sc.c
    ${CMAKE_SOURCE_DIR}/ed25519/keypair.c
    ${CMAKE_SOURCE_DIR}/ed25519/sha512_bridge.cpp
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/ed25519/fe.c
    ${CMAKE_SOURCE_DIR}/ed25519/ge.c
    ${CMAKE_SOURCE_DIR}/ed25519/sc.c
    ${CMAKE_SOURCE_DIR}/ed25519/keypair.c
    PROPERTIES COMPILE_DEFINITIONS "ED25519_NO_SEED"
)

if(IS_X86 AND ENABLE_SSE AND NOT IS_TERMUX)
    list(APPEND CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/hash/ripemd160_sse.cpp
        ${CMAKE_SOURCE_DIR}/hash/sha256_sse.cpp
    )
endif()

if(IS_X86 AND ENABLE_AVX512 AND NOT IS_TERMUX)
    list(APPEND CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/hash/hash160_avx512.cpp
        ${CMAKE_SOURCE_DIR}/hash/hash160_avx2.cpp
    )
endif()

if(IS_X86 AND ENABLE_AVX512 AND NOT IS_TERMUX)
    if(MSVC)
        set_source_files_properties(${CMAKE_SOURCE_DIR}/hash/hash160_avx512.cpp
            PROPERTIES COMPILE_OPTIONS "/arch:AVX512")
        set_source_files_properties(${CMAKE_SOURCE_DIR}/hash/hash160_avx2.cpp
            PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
    else()
        set_source_files_properties(${CMAKE_SOURCE_DIR}/hash/hash160_avx512.cpp
            PROPERTIES COMPILE_FLAGS "-mavx512f -mavx512bw")
        set_source_files_properties(${CMAKE_SOURCE_DIR}/hash/hash160_avx2.cpp
            PROPERTIES COMPILE_FLAGS "-mavx2")
    endif()
endif()

if(MSVC)
    list(APPEND CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/compat/getopt.c
    )
    include_directories(${CMAKE_SOURCE_DIR}/compat)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------------
add_executable(keyhunt ${CORE_SOURCES})

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
if(MSVC)
    set(PLATFORM_LIBS ws2_32)
elseif(WIN32)
    # ws2_32 needed for base58/ntohl; winpthread for pthread on MinGW.
    set(PLATFORM_LIBS ws2_32 winpthread)
    if(MINGW AND NOT STATIC_BUILD)
        target_link_options(keyhunt PRIVATE -static-libgcc -static-libstdc++)
    endif()
else()
    set(PLATFORM_LIBS m pthread)
endif()

target_link_libraries(keyhunt ${PLATFORM_LIBS})

# ---------------------------------------------------------------------------
# Optional GPU backends (scaffolding for future work)
# ---------------------------------------------------------------------------
if(ENABLE_CUDA)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    add_compile_definitions(ENABLE_CUDA)
    add_subdirectory(gpu/cuda)
endif()

if(ENABLE_OPENCL)
    add_compile_definitions(ENABLE_OPENCL)
    add_subdirectory(gpu/opencl)
endif()

# ---------------------------------------------------------------------------
# Small verification tests
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    add_subdirectory(tests_cmake)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS keyhunt RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# Print build summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "TrueCollider build summary")
message(STATUS "  System:        ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Processor:     ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  SSE enabled:   ${ENABLE_SSE}")
message(STATUS "  AVX detection: ${ENABLE_AVX}")
message(STATUS "  AVX-512 detection: ${ENABLE_AVX512}")
message(STATUS "  Static build:  ${STATIC_BUILD}")
message(STATUS "  CUDA backend:  ${ENABLE_CUDA}")
message(STATUS "  OpenCL backend: ${ENABLE_OPENCL}")
message(STATUS "  Tests:         ${BUILD_TESTS}")
message(STATUS "")
