# ed25519 (vendored)

Portable Ed25519 from [orlp/ed25519](https://github.com/orlp/ed25519) (zlib-style license — see `license.txt`).

Used for Solana (`-c sol`): 32-byte seed → clamped SHA-512 scalar → base-point multiply → 32-byte pubkey → base58 address.

`sha512` is provided by `ed25519_sha512_bridge.h` / `sha512_bridge.cpp` (forwards to `hash/sha512.cpp`) so symbols do not clash.
