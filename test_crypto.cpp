#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "secp256k1/SECP256K1.h"
#include "secp256k1/Int.h"
#include "secp256k1/Point.h"
#include "hash/sha256.h"
#include "hash/ripemd160.h"

int main() {
    Secp256K1 *secp = new Secp256K1();
    secp->Init();

    // Private key = 1
    Int key;
    key.SetInt32(1);
    Point pub = secp->ComputePublicKey(&key);

    printf("Private key: 1\n");
    printf("Public key X: %s\n", pub.x.GetBase16());
    printf("Public key Y: %s\n", pub.y.GetBase16());
    printf("Public key Y odd: %d\n", pub.y.IsOdd());

    // Compute hash160
    uint8_t hash160[20];
    secp->GetHash160(P2PKH, true, pub, hash160);

    printf("Hash160: ");
    for(int i = 0; i < 20; i++) printf("%02x", hash160[i]);
    printf("\n");

    // Compare with expected
    const char *expected = "751e76e8199196d454941c45d1b3a323f1433bd6";
    uint8_t exp[20];
    for(int i = 0; i < 20; i++) {
        unsigned int b;
        sscanf(expected + i*2, "%02x", &b);
        exp[i] = b;
    }

    if(memcmp(hash160, exp, 20) == 0) {
        printf("MATCH! Crypto is working correctly.\n");
    } else {
        printf("MISMATCH! Expected: %s\n", expected);
    }

    delete secp;
    return 0;
}
