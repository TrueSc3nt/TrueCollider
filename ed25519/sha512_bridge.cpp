/*
 * C-callable sha512 for orlp/ed25519 keypair derivation.
 * Forwards to the project's C++ sha512() in hash/sha512.cpp.
 */
#define ED25519_SHA512_NO_MACRO
#include "ed25519_sha512_bridge.h"
#undef ED25519_SHA512_NO_MACRO

#include "hash/sha512.h"

extern "C" void ed25519_compat_sha512(const unsigned char *message, size_t message_len, unsigned char *out) {
	sha512((unsigned char *)message, (int)message_len, out);
}
