#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "secp256k1/SECP256K1.h"
#include "secp256k1/Int.h"
#include "secp256k1/Point.h"
#include "hash/sha256.h"
#include "hash/ripemd160.h"
#include "oldbloom/oldbloom.h"
#include "bloom/bloom.h"
#include "binaryfuse/binaryfuse_wrapper.h"

int main() {
    Secp256K1 *secp = new Secp256K1();
    secp->Init();

    // Load target from file
    FILE *f = fopen("tests/test_privkey1.rmd", "r");
    if(!f) { printf("Cannot open file\n"); return 1; }
    uint8_t target[20];
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // Remove trailing newline
    line[strcspn(line, "\r\n")] = 0;
    printf("Target hex: %s\n", line);
    for(int i = 0; i < 20; i++) {
        unsigned int b;
        sscanf(line + i*2, "%02x", &b);
        target[i] = b;
    }
    printf("Target bin: ");
    for(int i = 0; i < 20; i++) printf("%02x", target[i]);
    printf("\n");

    // Initialize bloom filter with this one target
    struct bloom bloom;
    bloom_init(&bloom, 10000, 0.000001);
    bloom_add(&bloom, target, 20);
    
    struct bloom_filter bf;
    bf_init(&bf, 10000, 0.000001);
    bf_add(&bf, target, 20);

    // Compute hash160 for key=1
    Int key;
    key.SetInt32(1);
    Point pub = secp->ComputePublicKey(&key);
    uint8_t hash160[20];
    secp->GetHash160(P2PKH, true, pub, hash160);

    printf("\nKey 1 hash160: ");
    for(int i = 0; i < 20; i++) printf("%02x", hash160[i]);
    printf("\n");

    // Check bloom
    int r = bloom_check(&bloom, hash160, 20);
    printf("Bloom check: %d\n", r);
    
    // Check bf
    r = bf_check(&bf, hash160, 20);
    printf("BF check: %d\n", r);

    // Direct compare
    printf("Direct compare: %d\n", memcmp(hash160, target, 20) == 0);

    bloom_free(&bloom);
    bf_free(&bf);
    delete secp;
    return 0;
}
