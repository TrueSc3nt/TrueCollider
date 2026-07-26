#!/usr/bin/env bash
# Build keyhunt.exe natively with MinGW-w64 (MSYS2).
set -euo pipefail
export PATH="/mingw64/bin:/usr/bin:/bin:$PATH"
cd "$(dirname "$0")/.."

if [[ ! -f keyhunt.cpp ]]; then
  echo "[E] keyhunt.cpp missing - sources look incomplete/corrupt."
  echo "    Re-clone from https://github.com/TrueSc3nt/TrueCollider"
  exit 1
fi
if [[ ! -s Makefile ]]; then
  echo "[E] Makefile missing or empty/corrupt."
  exit 1
fi

echo "[+] Clean..."
rm -f keyhunt keyhunt.exe keyhunt_nolto.o *.o hash/*.o gpu/*.o ed25519/*.o 2>/dev/null || true

MAKE=mingw32-make
if ! command -v mingw32-make >/dev/null 2>&1; then
  if command -v make >/dev/null 2>&1; then
    MAKE=make
  else
    echo "[E] mingw32-make / make not found. Install: pacman -S mingw-w64-x86_64-make"
    exit 1
  fi
fi

echo "[+] Compiling with MinGW g++ via $MAKE..."
"$MAKE" -j"$(nproc 2>/dev/null || echo 4)" \
  OS=MINGW64 ARCH=x86_64 \
  CXX=g++ CC=gcc \
  TARGET=keyhunt.exe \
  "LIBS=-lws2_32 -lwinpthread -static -static-libgcc -static-libstdc++" \
  "CXXFLAGS=-Wall -Wextra -Wno-deprecated-copy -O2 -I. -m64 -mssse3 -ftree-vectorize -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" \
  "CFLAGS=-Wall -Wextra -O2 -I. -m64 -mssse3 -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" \
  "IS_X86=x86_64" \
  "IS_TERMUX=" \
  "SSE_DEFINE=-DHAVE_SSE"

if [[ ! -f keyhunt.exe ]]; then
  echo "[E] keyhunt.exe not produced"
  exit 1
fi
if [[ ! -s keyhunt.exe ]]; then
  echo "[E] keyhunt.exe is 0 bytes - corrupt output. Delete it and rebuild."
  rm -f keyhunt.exe
  exit 1
fi

echo "[+] Build OK: $(pwd)/keyhunt.exe ($(wc -c < keyhunt.exe) bytes)"
ls -lh keyhunt.exe
./keyhunt.exe -h | head -40 || true
echo "PASS: MinGW native build"
