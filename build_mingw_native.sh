#!/usr/bin/env bash
# Build keyhunt.exe natively with MinGW-w64 (MSYS2).
set -e
export PATH="/mingw64/bin:/usr/bin:/bin:$PATH"
cd "$(dirname "$0")"

echo "[+] Clean..."
rm -f keyhunt keyhunt.exe keyhunt_nolto.o *.o hash/*.o gpu/*.o 2>/dev/null || true

echo "[+] Compiling with MinGW g++..."
mingw32-make -j"$(nproc 2>/dev/null || echo 4)" \
  OS=MINGW64 ARCH=x86_64 \
  CXX=g++ CC=gcc \
  TARGET=keyhunt.exe \
  "LIBS=-lws2_32 -lwinpthread -static -static-libgcc -static-libstdc++" \
  "CXXFLAGS=-Wall -Wextra -Wno-deprecated-copy -O2 -I. -m64 -mssse3 -ftree-vectorize -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" \
  "CFLAGS=-Wall -Wextra -O2 -I. -m64 -mssse3 -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" \
  "IS_X86=x86_64" \
  "IS_TERMUX=" \
  "SSE_DEFINE=-DHAVE_SSE"

if [[ -f keyhunt.exe ]]; then
  echo "[+] Build OK: $(pwd)/keyhunt.exe"
  ls -lh keyhunt.exe
  ./keyhunt.exe -h | head -40
else
  echo "[E] keyhunt.exe not produced"
  exit 1
fi
