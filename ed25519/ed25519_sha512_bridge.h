/*
 * Bridge header: ed25519 sources expect sha512().
 * Implementation lives in sha512_bridge.cpp (uses project hash/sha512.cpp).
 * Altered from orlp/ed25519 — original sha512.c is not used (symbol clash).
 *
 * Named uniquely so it does not collide with hash/sha512.h on the include path.
 */
#ifndef ED25519_SHA512_BRIDGE_H
#define ED25519_SHA512_BRIDGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void ed25519_compat_sha512(const unsigned char *message, size_t message_len, unsigned char *out);

#ifdef __cplusplus
}
#endif

#ifndef ED25519_SHA512_NO_MACRO
#define sha512(message, message_len, out) ed25519_compat_sha512((message), (message_len), (out))
#endif

#endif
