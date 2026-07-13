#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <time.h>

#include "../hash/sha256.h"
#include "../hash/sha512.h"

// BIP-39 test vectors (English wordlist)
// Format: entropy_hex -> expected_mnemonic
struct BIP39TestVector {
    const char *entropy_hex;
    const char *expected_mnemonic;
    int word_count;
};

// Official BIP-39 test vectors from spec
static const BIP39TestVector test_vectors[] = {
    {"00000000000000000000000000000000", "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", 12},
    {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f", "legal winner thank year wave sausage worth useful legal winner thank yellow", 12},
    {"80808080808080808080808080808080", "letter advice cage absurd amount doctor acoustic avoid letter advice cage above", 12},
    {"ffffffffffffffffffffffffffffffff", "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong", 12},
    {"000000000000000000000000000000000000000000000000", "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent", 18},
    {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f", "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will", 18},
    {"808080808080808080808080808080808080808080808080", "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always", 18},
    {"ffffffffffffffffffffffffffffffffffffffffffffffff", "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when", 18},
    {"0000000000000000000000000000000000000000000000000000000000000000", "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art", 24},
    {"7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f", "legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title", 24},
    {"8080808080808080808080808080808080808080808080808080808080808080", "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless", 24},
    {"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote", 24},
};

static const int NUM_TEST_VECTORS = sizeof(test_vectors) / sizeof(test_vectors[0]);

// Load English wordlist
static char *wordlist[2048];
static int wordlist_size = 0;

bool load_wordlist(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[E] Cannot open wordlist: %s\n", path);
        return false;
    }
    char line[64];
    wordlist_size = 0;
    while (fgets(line, sizeof(line), f) && wordlist_size < 2048) {
        // Remove newline
        for (int i = 0; line[i]; i++) {
            if (line[i] == '\n' || line[i] == '\r') {
                line[i] = '\0';
                break;
            }
        }
        if (strlen(line) > 0) {
            wordlist[wordlist_size] = strdup(line);
            wordlist_size++;
        }
    }
    fclose(f);
    return (wordlist_size == 2048);
}

// Convert hex string to bytes
int hex_to_bytes(const char *hex, uint8_t *bytes, int max_bytes) {
    int len = strlen(hex);
    if (len % 2 != 0 || len / 2 > max_bytes) return -1;
    int byte_count = len / 2;
    for (int i = 0; i < byte_count; i++) {
        unsigned int val;
        if (sscanf(hex + i * 2, "%02x", &val) != 1) return -1;
        bytes[i] = (uint8_t)val;
    }
    return byte_count;
}

// Generate mnemonic from entropy (same logic as keyhunt)
bool generate_mnemonic_from_entropy(const uint8_t *entropy, int entropy_bytes, char *output) {
    if (wordlist_size != 2048) return false;

    int strength = entropy_bytes * 8;
    int checksum_bits = strength / 32;
    int total_bits = strength + checksum_bits;
    int word_count = total_bits / 11;

    uint8_t hash[32];
    sha256((uint8_t *)entropy, entropy_bytes, hash);

    char bits[288];
    int bit_idx = 0;
    for (int i = 0; i < entropy_bytes; i++) {
        for (int b = 7; b >= 0; b--) {
            bits[bit_idx++] = (entropy[i] >> b) & 1;
        }
    }
    for (int i = 0; i < checksum_bits; i++) {
        bits[bit_idx++] = (hash[i / 8] >> (7 - (i % 8))) & 1;
    }

    output[0] = '\0';
    for (int i = 0; i < word_count; i++) {
        int idx = 0;
        for (int b = 0; b < 11; b++) {
            idx = (idx << 1) | bits[i * 11 + b];
        }
        if (idx < 0 || idx >= wordlist_size) {
            output[0] = '\0';
            return false;
        }
        if (i > 0) strcat(output, " ");
        strcat(output, wordlist[idx]);
    }
    return true;
}

// Validate mnemonic checksum (same logic as keyhunt)
bool validate_mnemonic(const char *mnemonic) {
    if (wordlist_size != 2048 || !mnemonic || !mnemonic[0]) return false;

    char temp[512];
    strncpy(temp, mnemonic, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    int word_count = 0;
    char *words[24];
    char *saveptr;
    char *tok = strtok_r(temp, " ", &saveptr);
    while (tok && word_count < 24) {
        words[word_count++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }

    if (word_count != 12 && word_count != 15 && word_count != 18 && word_count != 21 && word_count != 24)
        return false;

    char bits[288];
    int bit_idx = 0;
    for (int i = 0; i < word_count; i++) {
        int idx = -1;
        for (int j = 0; j < 2048; j++) {
            if (strcmp(words[i], wordlist[j]) == 0) {
                idx = j;
                break;
            }
        }
        if (idx < 0) return false;
        for (int b = 10; b >= 0; b--) {
            bits[bit_idx++] = (idx >> b) & 1;
        }
    }

    int strength = word_count * 11 - (word_count * 11) / 32;
    int entropy_bytes = strength / 8;
    int checksum_bits = strength / 32;

    uint8_t entropy[32] = {0};
    for (int i = 0; i < entropy_bytes; i++) {
        for (int b = 0; b < 8; b++) {
            entropy[i] |= (bits[i * 8 + b] << (7 - b));
        }
    }

    uint8_t hash[32];
    sha256(entropy, entropy_bytes, hash);

    for (int i = 0; i < checksum_bits; i++) {
        int data_bit = bits[entropy_bytes * 8 + i];
        int hash_bit = (hash[i / 8] >> (7 - (i % 8))) & 1;
        if (data_bit != hash_bit) return false;
    }

    return true;
}

// Secure random bytes (same as keyhunt for testing)
static unsigned int rng_state;
static void test_random_bytes(uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        rng_state = rng_state * 1103515245 + 12345;
        buf[i] = (uint8_t)((rng_state >> 16) & 0xFF);
    }
}

// Generate mnemonic with checksum (same as keyhunt)
bool test_generate_mnemonic(char *output, int word_count) {
    if (wordlist_size != 2048) return false;

    int strength;
    switch (word_count) {
        case 12: strength = 128; break;
        case 15: strength = 160; break;
        case 18: strength = 192; break;
        case 21: strength = 224; break;
        case 24: strength = 256; break;
        default: return false;
    }

    int entropy_bytes = strength / 8;
    int checksum_bits = strength / 32;
    int total_bits = strength + checksum_bits;
    int calc_word_count = total_bits / 11;

    uint8_t entropy[32];
    test_random_bytes(entropy, entropy_bytes);

    uint8_t hash[32];
    sha256(entropy, entropy_bytes, hash);

    char bits[288];
    int bit_idx = 0;
    for (int i = 0; i < entropy_bytes; i++) {
        for (int b = 7; b >= 0; b--) {
            bits[bit_idx++] = (entropy[i] >> b) & 1;
        }
    }
    for (int i = 0; i < checksum_bits; i++) {
        bits[bit_idx++] = (hash[i / 8] >> (7 - (i % 8))) & 1;
    }

    output[0] = '\0';
    for (int i = 0; i < calc_word_count; i++) {
        int idx = 0;
        for (int b = 0; b < 11; b++) {
            idx = (idx << 1) | bits[i * 11 + b];
        }
        if (idx < 0 || idx >= wordlist_size) {
            output[0] = '\0';
            return false;
        }
        if (i > 0) strcat(output, " ");
        strcat(output, wordlist[idx]);
    }
    return true;
}

int main() {
    printf("=============================================================\n");
    printf("  BIP-39 Mnemonic Checksum Verification Test Suite\n");
    printf("=============================================================\n\n");

    // Load wordlist
    if (!load_wordlist("bip39/english.txt")) {
        fprintf(stderr, "[E] Failed to load English wordlist\n");
        return 1;
    }
    printf("[+] Loaded %d English words\n\n", wordlist_size);

    int passed = 0;
    int failed = 0;

    // ============================================================
    // TEST 1: Official BIP-39 test vectors (entropy -> mnemonic)
    // ============================================================
    printf("=== TEST 1: Official BIP-39 Test Vectors ===\n");
    for (int i = 0; i < NUM_TEST_VECTORS; i++) {
        uint8_t entropy[32];
        int entropy_bytes = hex_to_bytes(test_vectors[i].entropy_hex, entropy, 32);
        if (entropy_bytes < 0) {
            printf("[FAIL] Vector %d: invalid hex\n", i + 1);
            failed++;
            continue;
        }

        char mnemonic[512];
        if (!generate_mnemonic_from_entropy(entropy, entropy_bytes, mnemonic)) {
            printf("[FAIL] Vector %d: generation failed\n", i + 1);
            failed++;
            continue;
        }

        if (strcmp(mnemonic, test_vectors[i].expected_mnemonic) == 0) {
            printf("[PASS] Vector %2d (%2d words): %s\n", i + 1, test_vectors[i].word_count,
                   test_vectors[i].expected_mnemonic);
            passed++;
        } else {
            printf("[FAIL] Vector %2d (%2d words):\n", i + 1, test_vectors[i].word_count);
            printf("  Expected: %s\n", test_vectors[i].expected_mnemonic);
            printf("  Got:      %s\n", mnemonic);
            failed++;
        }
    }
    printf("\n");

    // ============================================================
    // TEST 2: Checksum validation of known-good mnemonics
    // ============================================================
    printf("=== TEST 2: Validate Known-Good Mnemonics ===\n");
    for (int i = 0; i < NUM_TEST_VECTORS; i++) {
        if (validate_mnemonic(test_vectors[i].expected_mnemonic)) {
            printf("[PASS] Vector %2d: checksum VALID\n", i + 1);
            passed++;
        } else {
            printf("[FAIL] Vector %2d: checksum INVALID (should be valid!)\n", i + 1);
            failed++;
        }
    }
    printf("\n");

    // ============================================================
    // TEST 3: Reject known-bad mnemonics (wrong checksum)
    // ============================================================
    printf("=== TEST 3: Reject Bad Checksums ===\n");

    // Change last word to break checksum
    const char *bad_mnemonics[] = {
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon zoo",
        "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon",
        "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo about",
        "letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd",
    };
    const int NUM_BAD = sizeof(bad_mnemonics) / sizeof(bad_mnemonics[0]);

    for (int i = 0; i < NUM_BAD; i++) {
        if (!validate_mnemonic(bad_mnemonics[i])) {
            printf("[PASS] Bad mnemonic %d: checksum INVALID (correctly rejected)\n", i + 1);
            passed++;
        } else {
            printf("[FAIL] Bad mnemonic %d: checksum VALID (should be invalid!)\n", i + 1);
            failed++;
        }
    }
    printf("\n");

    // ============================================================
    // TEST 4: Generate + Validate 10000 random mnemonics
    // ============================================================
    printf("=== TEST 4: Generate & Validate 10000 Random Mnemonics ===\n");
    rng_state = (unsigned int)time(NULL);

    int gen_passed = 0;
    int gen_failed = 0;
    int word_counts[] = {12, 15, 18, 21, 24};

    for (int i = 0; i < 10000; i++) {
        int wc = word_counts[i % 5];
        char mnemonic[512];

        if (!test_generate_mnemonic(mnemonic, wc)) {
            printf("[FAIL] Gen %d: generation failed\n", i + 1);
            gen_failed++;
            continue;
        }

        // Verify word count
        int actual_words = 1;
        for (char *p = mnemonic; *p; p++) {
            if (*p == ' ') actual_words++;
        }
        if (actual_words != wc) {
            printf("[FAIL] Gen %d: expected %d words, got %d\n", i + 1, wc, actual_words);
            gen_failed++;
            continue;
        }

        // Verify checksum
        if (validate_mnemonic(mnemonic)) {
            gen_passed++;
        } else {
            printf("[FAIL] Gen %d (%d words): checksum INVALID!\n", i + 1, wc);
            printf("  Mnemonic: %s\n", mnemonic);
            gen_failed++;
        }
    }

    if (gen_failed == 0) {
        printf("[PASS] All %d generated mnemonics have valid checksums\n", gen_passed);
        passed++;
    } else {
        printf("[FAIL] %d/%d generated mnemonics failed checksum\n", gen_failed, gen_passed + gen_failed);
        failed++;
    }
    printf("\n");

    // ============================================================
    // TEST 5: Cross-verify generation and validation
    // ============================================================
    printf("=== TEST 5: Cross-Verify Generate -> Validate -> Regenerate ===\n");
    rng_state = (unsigned int)time(NULL) ^ 0xDEADBEEF;
    int cross_passed = 0;
    int cross_failed = 0;

    for (int i = 0; i < 1000; i++) {
        uint8_t entropy[32];
        test_random_bytes(entropy, 16);

        char mnemonic1[512];
        if (!generate_mnemonic_from_entropy(entropy, 16, mnemonic1)) {
            cross_failed++;
            continue;
        }

        // Validate
        if (!validate_mnemonic(mnemonic1)) {
            printf("[FAIL] Cross %d: generated mnemonic fails validation\n", i + 1);
            cross_failed++;
            continue;
        }

        // Regenerate from same entropy
        char mnemonic2[512];
        if (!generate_mnemonic_from_entropy(entropy, 16, mnemonic2)) {
            cross_failed++;
            continue;
        }

        if (strcmp(mnemonic1, mnemonic2) == 0) {
            cross_passed++;
        } else {
            printf("[FAIL] Cross %d: regeneration mismatch\n", i + 1);
            cross_failed++;
        }
    }

    if (cross_failed == 0) {
        printf("[PASS] All %d cross-verification tests passed\n", cross_passed);
        passed++;
    } else {
        printf("[FAIL] %d/%d cross-verification tests failed\n", cross_failed, cross_passed + cross_failed);
        failed++;
    }
    printf("\n");

    // ============================================================
    // TEST 6: Verify PBKDF2-HMAC-SHA512 seed derivation
    // ============================================================
    printf("=== TEST 6: PBKDF2-HMAC-SHA512 Seed Derivation ===\n");

    // Official test vector: mnemonic "abandon x11 about" + passphrase "" -> seed
    // Source: https://github.com/trezor/python-mnemonic/blob/master/vectors.json
    const char *test_mnemonic = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
    const char *test_passphrase = "";
    uint8_t expected_seed[] = {
        0x5e, 0xb0, 0x0b, 0xbd, 0xdc, 0xf0, 0x69, 0x08,
        0x48, 0x89, 0xa8, 0xab, 0x91, 0x55, 0x56, 0x81,
        0x65, 0xf5, 0xc4, 0x53, 0xcc, 0xb8, 0x5e, 0x70,
        0x81, 0x1a, 0xae, 0xd6, 0xf6, 0xda, 0x5f, 0xc1,
        0x9a, 0x5a, 0xc4, 0x0b, 0x38, 0x9c, 0xd3, 0x70,
        0xd0, 0x86, 0x20, 0x6d, 0xec, 0x8a, 0xa6, 0xc4,
        0x3d, 0xae, 0xa6, 0x69, 0x0f, 0x20, 0xad, 0x3d,
        0x8d, 0x48, 0xb2, 0xd2, 0xce, 0x9e, 0x38, 0xe4
    };

    char salt[256];
    snprintf(salt, sizeof(salt), "mnemonic%s", test_passphrase);
    uint8_t seed[64];
    pbkdf2_hmac_sha512(seed, 64,
                       (uint8_t *)test_mnemonic, strlen(test_mnemonic),
                       (uint8_t *)salt, strlen(salt), 2048);

    if (memcmp(seed, expected_seed, 64) == 0) {
        printf("[PASS] PBKDF2-HMAC-SHA512 seed matches BIP-39 test vector\n");
        passed++;
    } else {
        printf("[FAIL] PBKDF2-HMAC-SHA512 seed mismatch!\n");
        printf("  Expected: ");
        for (int i = 0; i < 64; i++) printf("%02x", expected_seed[i]);
        printf("\n  Got:      ");
        for (int i = 0; i < 64; i++) printf("%02x", seed[i]);
        printf("\n");
        failed++;
    }
    printf("\n");

    // ============================================================
    // TEST 7: Verify BIP-32 master key derivation
    // ============================================================
    printf("=== TEST 7: BIP-32 Master Key Derivation ===\n");

    // From BIP-32 test vector 1
    // Seed: 000102030405060708090a0b0c0d0e0f
    // Master private key: e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35
    // Master chain code: 873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508

    uint8_t test_seed2[16] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t master_key[32], master_chain[32];
    uint8_t hmac[64];

    hmac_sha512((uint8_t *)"Bitcoin seed", 12, test_seed2, 16, hmac);
    memcpy(master_key, hmac, 32);
    memcpy(master_chain, hmac + 32, 32);

    uint8_t expected_mkey[] = {0xe8,0xf3,0x2e,0x72,0x3d,0xec,0xf4,0x05,0x1a,0xef,0xac,0x8e,0x2c,0x93,0xc9,0xc5,0xb2,0x14,0x31,0x38,0x17,0xcd,0xb0,0x1a,0x14,0x94,0xb9,0x17,0xc8,0x43,0x6b,0x35};
    uint8_t expected_mchain[] = {0x87,0x3d,0xff,0x81,0xc0,0x2f,0x52,0x56,0x23,0xfd,0x1f,0xe5,0x16,0x7e,0xac,0x3a,0x55,0xa0,0x49,0xde,0x3d,0x31,0x4b,0xb4,0x2e,0xe2,0x27,0xff,0xed,0x37,0xd5,0x08};

    int key_ok = (memcmp(master_key, expected_mkey, 32) == 0);
    int chain_ok = (memcmp(master_chain, expected_mchain, 32) == 0);

    if (key_ok && chain_ok) {
        printf("[PASS] BIP-32 master key and chain code match test vector\n");
        passed++;
    } else {
        if (!key_ok) {
            printf("[FAIL] BIP-32 master key mismatch!\n");
            printf("  Expected: ");
            for (int i = 0; i < 32; i++) printf("%02x", expected_mkey[i]);
            printf("\n  Got:      ");
            for (int i = 0; i < 32; i++) printf("%02x", master_key[i]);
            printf("\n");
        }
        if (!chain_ok) {
            printf("[FAIL] BIP-32 master chain code mismatch!\n");
            printf("  Expected: ");
            for (int i = 0; i < 32; i++) printf("%02x", expected_mchain[i]);
            printf("\n  Got:      ");
            for (int i = 0; i < 32; i++) printf("%02x", master_chain[i]);
            printf("\n");
        }
        failed++;
    }
    printf("\n");

    // ============================================================
    // SUMMARY
    // ============================================================
    printf("=============================================================\n");
    printf("  RESULTS: %d PASSED, %d FAILED\n", passed, failed);
    printf("=============================================================\n\n");

    if (failed == 0) {
        printf("ALL TESTS PASSED - BIP-39 checksum generation and validation are 100%% correct.\n");
    } else {
        printf("SOME TESTS FAILED - Review output above.\n");
    }

    // Cleanup
    for (int i = 0; i < wordlist_size; i++) {
        free(wordlist[i]);
    }

    return failed;
}
