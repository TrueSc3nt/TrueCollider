# TrueCollider cross-platform Makefile
# Auto-detects architecture and compiler flags
# Works on: Linux x86_64, Linux ARM64, Termux, macOS, MinGW, WSL

# Detect OS
OS := $(shell uname -s 2>/dev/null || echo Windows)
# Detect architecture
ARCH := $(shell uname -m 2>/dev/null || echo x86_64)

CXX := g++
CC  := gcc
CPU_GRP_SIZE ?= 1024

CXXFLAGS_BASE := -Wall -Wextra -Wno-deprecated-copy -O2 -I. -DCPU_GRP_SIZE=$(CPU_GRP_SIZE)
CFLAGS_BASE   := -Wall -Wextra -Wno-unused-parameter -O2 -I. -DCPU_GRP_SIZE=$(CPU_GRP_SIZE)

# Detect x86 vs ARM vs other
IS_X86 := $(filter x86_64 i386 i686 amd64,$(ARCH))
IS_ARM := $(filter aarch64 arm64 armv7l armv8l,$(ARCH))

# Base flags (no arch-specific stuff)
CXXFLAGS := $(CXXFLAGS_BASE)
CFLAGS   := $(CFLAGS_BASE)

ifeq ($(IS_X86),)
  # Not x86 — ARM or other arch (Termux, Raspberry Pi, Apple Silicon, etc.)
  # Do NOT use -m64, -mssse3, -march=native on non-x86
  CXXFLAGS += -DNO_SSE
  CFLAGS   += -DNO_SSE
else
  # x86 detected — safe to use SSE/native flags
  CXXFLAGS += -m64 -march=native -mtune=native -mssse3 -ftree-vectorize
  CFLAGS   += -m64 -march=native -mtune=native -mssse3 -ftree-vectorize
endif

# Platform-specific flags
ifeq ($(OS),Darwin)
  CXXFLAGS += -DOS_MACOS
  CFLAGS   += -DOS_MACOS
else ifneq (,$(findstring MINGW,$(OS)))
  CXXFLAGS += -DOS_WINDOWS
  CFLAGS   += -DOS_WINDOWS
else ifneq (,$(findstring MSYS,$(OS)))
  CXXFLAGS += -DOS_WINDOWS
  CFLAGS   += -DOS_WINDOWS
else ifneq (,$(findstring CYGWIN,$(OS)))
  CXXFLAGS += -DOS_CYGWIN
  CFLAGS   += -DOS_CYGWIN
endif

# Detect Termux (Unix shells only — cmd.exe has no `test`)
ifneq ($(filter Windows% MINGW% MSYS% CYGWIN%,$(OS)),)
  IS_TERMUX :=
else ifeq ($(OS),Windows)
  IS_TERMUX :=
else
  IS_TERMUX := $(shell test -d /data/data/com.termux 2>/dev/null && echo yes)
endif
ifeq ($(IS_TERMUX),yes)
  CXXFLAGS += -DTERMUX -DNO_SSE
  CFLAGS   += -DTERMUX -DNO_SSE
  # Termux has broken SSE — disable all SSE/native flags
  CXXFLAGS := $(filter-out -mssse3 -march=native -mtune=native -m64,$(CXXFLAGS))
  CFLAGS   := $(filter-out -mssse3 -march=native -mtune=native -m64,$(CFLAGS))
  CXXFLAGS += -mno-sse -mno-ssse3 -mno-sse2
  CFLAGS   += -mno-sse -mno-ssse3 -mno-sse2
endif

LIBS := -lm -lpthread

OBJECTS := research_engine.o research_implants.o oldbloom.o bloom.o base58.o rmd160.o sha3.o keccak.o xxhash.o \
           util.o backend_config.o cpu_features.o Int.o Point.o SECP256K1.o \
           IntMod.o Random.o IntGroup.o gpu/gpu_dispatcher.o \
           hash/ripemd160.o hash/sha256.o hash/sha512.o \
           ed25519/fe.o ed25519/ge.o ed25519/sc.o ed25519/keypair.o \
           ed25519/sha512_bridge.o

# On x86 (non-Termux), also compile SSE hash files
ifneq ($(IS_X86),)
  ifneq ($(IS_TERMUX),yes)
    OBJECTS += hash/ripemd160_sse.o hash/sha256_sse.o hash/hash160_avx512.o hash/hash160_avx2.o
    SSE_DEFINE := -DHAVE_SSE
  else
    SSE_DEFINE := -DNO_SSE
  endif
else
  SSE_DEFINE := -DNO_SSE
endif

TARGET := keyhunt

default: $(TARGET)

# Windows cross-compile target (Linux/WSL -> MinGW x86_64 .exe)
# Builds a static binary that runs on Windows without extra DLLs.
# NOTE: must not be the first target (would recurse as default goal).
windows:
	@echo "[+] Cross-compiling Windows x86_64 .exe with MinGW..."
	$(MAKE) clean
	$(MAKE) OS=MINGW64 ARCH=win64 CXX=x86_64-w64-mingw32-g++ CC=x86_64-w64-mingw32-gcc \
		TARGET=keyhunt.exe "LIBS=-lm -lwinpthread -lws2_32 -static" \
		"CXXFLAGS_BASE=-Wall -Wextra -O2 -DCPU_GRP_SIZE=$(CPU_GRP_SIZE)" \
		"CFLAGS_BASE=-Wall -Wextra -O2 -DCPU_GRP_SIZE=$(CPU_GRP_SIZE)"

$(TARGET): keyhunt_nolto.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

keyhunt_nolto.o: keyhunt.cpp
	$(CXX) $(CXXFLAGS) $(SSE_DEFINE) -c $< -o $@

# secp256k1
Int.o: secp256k1/Int.cpp secp256k1/Int.h secp256k1/Random.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

Point.o: secp256k1/Point.cpp secp256k1/Point.h secp256k1/Int.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

SECP256K1.o: secp256k1/SECP256K1.cpp secp256k1/SECP256k1.h secp256k1/Point.h secp256k1/Int.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

IntMod.o: secp256k1/IntMod.cpp secp256k1/Int.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

Random.o: secp256k1/Random.cpp secp256k1/Random.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

IntGroup.o: secp256k1/IntGroup.cpp secp256k1/IntGroup.h secp256k1/Int.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# bloom filters
oldbloom.o: oldbloom/bloom.cpp oldbloom/oldbloom.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

bloom.o: bloom/bloom.cpp bloom/bloom.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C files
base58.o: base58/base58.c base58/libbase58.h
	$(CC) $(CFLAGS) -c $< -o $@

rmd160.o: rmd160/rmd160.c rmd160/rmd160.h
	$(CC) $(CFLAGS) -c $< -o $@

sha3.o: sha3/sha3.c sha3/sha3.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

keccak.o: sha3/keccak.c sha3/sha3.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

xxhash.o: xxhash/xxhash.c xxhash/xxhash.h
	$(CC) $(CFLAGS) -c $< -o $@

util.o: util.cpp util.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

backend_config.o: backend_config.cpp backend_config.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

cpu_features.o: cpu_features.cpp cpu_features.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

gpu/gpu_dispatcher.o: gpu/gpu_dispatcher.cpp gpu/gpu_dispatcher.h backend_config.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Hash files (non-SSE)
hash/ripemd160.o: hash/ripemd160.cpp hash/ripemd160.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

hash/sha256.o: hash/sha256.cpp hash/sha256.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

hash/sha512.o: hash/sha512.cpp hash/sha512.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ed25519 (Solana)
ed25519/fe.o: ed25519/fe.c ed25519/fe.h
	$(CC) $(CFLAGS) -Ied25519 -DED25519_NO_SEED -c $< -o $@

ed25519/ge.o: ed25519/ge.c ed25519/ge.h ed25519/fe.h ed25519/precomp_data.h
	$(CC) $(CFLAGS) -Ied25519 -DED25519_NO_SEED -c $< -o $@

ed25519/sc.o: ed25519/sc.c ed25519/sc.h
	$(CC) $(CFLAGS) -Ied25519 -DED25519_NO_SEED -c $< -o $@

ed25519/keypair.o: ed25519/keypair.c ed25519/ed25519.h ed25519/ed25519_sha512_bridge.h ed25519/ge.h
	$(CC) $(CFLAGS) -Ied25519 -DED25519_NO_SEED -c $< -o $@

ed25519/sha512_bridge.o: ed25519/sha512_bridge.cpp ed25519/ed25519_sha512_bridge.h hash/sha512.h
	$(CXX) $(CXXFLAGS) -Ied25519 -c $< -o $@

# SSE hash files (x86 non-Termux only)
ifneq ($(IS_X86),)
  ifneq ($(IS_TERMUX),yes)
hash/ripemd160_sse.o: hash/ripemd160_sse.cpp hash/ripemd160.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

hash/sha256_sse.o: hash/sha256_sse.cpp hash/sha256.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

hash/hash160_avx512.o: hash/hash160_avx512.cpp hash/hash160_avx512.h hash/sha256.h hash/ripemd160.h
	$(CXX) $(CXXFLAGS) -mavx512f -mavx512bw -c $< -o $@

hash/hash160_avx2.o: hash/hash160_avx2.cpp hash/hash160_avx2.h hash/sha256.h hash/ripemd160.h
	$(CXX) $(CXXFLAGS) -mavx2 -c $< -o $@
  endif
endif

.PHONY: default clean windows

clean:
ifneq ($(filter Windows% MINGW% MSYS% CYGWIN%,$(OS)),)
	-cmd /C "del /Q keyhunt.exe keyhunt keyhunt_nolto.o *.o 2>nul"
	-cmd /C "del /Q hash\*.o gpu\*.o ed25519\*.o 2>nul"
else ifeq ($(OS),Windows)
	-cmd /C "del /Q keyhunt.exe keyhunt keyhunt_nolto.o *.o 2>nul"
	-cmd /C "del /Q hash\*.o gpu\*.o ed25519\*.o 2>nul"
else
	rm -f $(TARGET) keyhunt.exe keyhunt_nolto.o *.o hash/*.o gpu/*.o ed25519/*.o
endif

research_engine.o: research_engine.cpp research_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

research_implants.o: research_implants.cpp research_engine.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
