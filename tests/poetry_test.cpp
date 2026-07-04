#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <math.h>

static char *poetry_words[2048];
static int poetry_words_size = 0;

bool load_words(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    char line[64];
    while (fgets(line, sizeof(line), f) && poetry_words_size < 2048) {
        for (int i = 0; line[i]; i++) if (line[i] == '\n' || line[i] == '\r') { line[i] = '\0'; break; }
        if (strlen(line) > 0) poetry_words[poetry_words_size++] = strdup(line);
    }
    fclose(f);
    return true;
}

int get_hex_per_group() {
    int bits_per_group = (int)(3.0 * log2((double)poetry_words_size));
    int hex_per_group = bits_per_group / 4;
    if (hex_per_group < 1) hex_per_group = 1;
    if (hex_per_group > 8) hex_per_group = 8;
    if (hex_per_group % 2 != 0) hex_per_group--;
    if (hex_per_group < 2) hex_per_group = 2;
    return hex_per_group;
}

void hex_to_poetry(const char *hex, char *output) {
    output[0] = '\0';
    int hex_len = strlen(hex);
    int hex_per_group = get_hex_per_group();
    for (int i = 0; i < hex_len; i += hex_per_group) {
        char chunk[9] = {0};
        strncpy(chunk, hex + i, hex_per_group);
        uint64_t x = strtoull(chunk, NULL, 16);
        int w1 = x % poetry_words_size;
        int w2 = (x / poetry_words_size) % poetry_words_size;
        int w3 = (x / ((uint64_t)poetry_words_size * poetry_words_size)) % poetry_words_size;
        if (i > 0) strcat(output, " ");
        strcat(output, poetry_words[w1]);
        strcat(output, " ");
        strcat(output, poetry_words[w2]);
        strcat(output, " ");
        strcat(output, poetry_words[w3]);
    }
}

void poetry_to_hex(const char *poetry, char *hex_output) {
    hex_output[0] = '\0';
    int hex_per_group = get_hex_per_group();
    char temp[2048];
    strncpy(temp, poetry, sizeof(temp));
    char *saveptr;
    char *word = strtok_r(temp, " ", &saveptr);
    int count = 0;
    while (word && count < 300) {
        int w1_idx = -1, w2_idx = -1, w3_idx = -1;
        for (int i = 0; i < poetry_words_size; i++) {
            if (strcmp(word, poetry_words[i]) == 0) { w1_idx = i; break; }
        }
        word = strtok_r(NULL, " ", &saveptr);
        if (!word) break;
        for (int i = 0; i < poetry_words_size; i++) {
            if (strcmp(word, poetry_words[i]) == 0) { w2_idx = i; break; }
        }
        word = strtok_r(NULL, " ", &saveptr);
        if (!word) break;
        for (int i = 0; i < poetry_words_size; i++) {
            if (strcmp(word, poetry_words[i]) == 0) { w3_idx = i; break; }
        }
        if (w1_idx >= 0 && w2_idx >= 0 && w3_idx >= 0) {
            uint64_t x = (uint64_t)w1_idx + (uint64_t)poetry_words_size * (uint64_t)w2_idx
                       + (uint64_t)poetry_words_size * (uint64_t)poetry_words_size * (uint64_t)w3_idx;
            char fmt[16];
            snprintf(fmt, sizeof(fmt), "%%0%dx", hex_per_group);
            sprintf(hex_output + strlen(hex_output), fmt, (uint32_t)x);
        }
        word = strtok_r(NULL, " ", &saveptr);
        count += 3;
    }
}

int main() {
    if (!load_words("poetry.txt")) {
        printf("Failed to load poetry.txt\n");
        return 1;
    }
    printf("[+] Loaded %d poetry words\n", poetry_words_size);

    int hpg = get_hex_per_group();
    int bits_per_group = hpg * 4;
    printf("[+] %d words: %d bits per 3-word group, %d hex chars per group\n\n", poetry_words_size, bits_per_group, hpg);

    // Test round-trip: hex -> poetry -> hex
    // Use hex strings that fit in the representable range
    const char *test_hexes[] = {
        "000000",
        "000001",
        "ffffff",
        "deadbe",
        "123456",
        "aabbcc",
        "800000",
        "7fffff",
    };
    int num_tests = sizeof(test_hexes) / sizeof(test_hexes[0]);
    int passed = 0, failed = 0;

    printf("=== Round-trip: hex -> poetry -> hex ===\n");
    for (int i = 0; i < num_tests; i++) {
        char poetry[512], hex_back[256] = {0};
        hex_to_poetry(test_hexes[i], poetry);
        poetry_to_hex(poetry, hex_back);

        if (strcmp(test_hexes[i], hex_back) == 0) {
            printf("[PASS] %s -> %s -> %s\n", test_hexes[i], poetry, hex_back);
            passed++;
        } else {
            printf("[FAIL] %s -> %s -> %s (expected %s)\n", test_hexes[i], poetry, hex_back, test_hexes[i]);
            failed++;
        }
    }

    // Test all values in representable range (sampled)
    uint64_t max_val = 1;
    for (int i = 0; i < bits_per_group; i++) max_val *= 2;
    printf("\n=== Sampling 100000 values in [0, %llu) ===\n", (unsigned long long)max_val);
    unsigned int seed = 42;
    int sample_pass = 0, sample_fail = 0;
    int fail_shown = 0;
    for (int i = 0; i < 100000; i++) {
        seed = seed * 1103515245 + 12345;
        uint32_t val = seed % (uint32_t)max_val;
        char hex[9] = {0}, poetry[512], hex_back[256] = {0};
        sprintf(hex, "%06x", val);
        hex_to_poetry(hex, poetry);
        poetry_to_hex(poetry, hex_back);
        if (strcmp(hex, hex_back) == 0) sample_pass++;
        else {
            sample_fail++;
            if (fail_shown < 5) {
                printf("  [DEBUG] val=%u hex='%s' poetry='%s' back='%s'\n", val, hex, poetry, hex_back);
                fail_shown++;
            }
        }
    }
    printf("[RESULT] %d passed, %d failed out of 100000\n", sample_pass, sample_fail);
    if (sample_fail == 0) {
        printf("[PASS] Encode/decode is 100%% reversible\n");
        passed++;
    } else {
        printf("[FAIL] Encode/decode is lossy - %d values don't round-trip\n", sample_fail);
        failed++;
    }

    printf("\n=== Summary ===\n");
    printf("Passed: %d, Failed: %d\n", passed, failed);
    return failed;
}
