#ifndef TRUECOLLIDER_RESEARCH_ENGINE_H
#define TRUECOLLIDER_RESEARCH_ENGINE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mnemonic / research submodes (-R) */
enum {
	RSUB_RANDOM = 0,
	RSUB_MASK,
	RSUB_MODEL,
	RSUB_LASTWORD,
	RSUB_PREFIX_WORD,
	RSUB_TYPO,
	RSUB_PERMUTE,
	RSUB_ANAGRAM,
	RSUB_POSITIONAL_SWAP,
	RSUB_LANGUAGE_GUESS,
	RSUB_MIXED_SCRIPT,
	RSUB_PASS_DICT,
	RSUB_PASS_MASK,
	RSUB_PASS_RULES,
	RSUB_PASS_HYBRID,
	RSUB_PASS_EMPTY_PLUS,
	RSUB_ELECTRUM_V1,
	RSUB_ELECTRUM_V2,
	RSUB_SLIP39,
	RSUB_AEZEED,
	RSUB_BIP85,
	RSUB_RFC1751,
	RSUB_SOLANA_BIP39,
	RSUB_MILKSAD,
	RSUB_LATTICE,
	RSUB_CHECKSUM_PRISM,
	RSUB_PATHS_BTC,
	RSUB_PATHS_ETH,
	RSUB_PATHS_ELECTRUM,
	RSUB_PATHS_CUSTOM,
	RSUB_ACCOUNT_SWEEP,
	RSUB_MULTISIG,
	RSUB_COUNT
};

/* Filter strategies (-F) */
enum {
	RFILTER_DEFAULT = 0,
	RFILTER_CASCADE,
	RFILTER_FUSE16,
	RFILTER_BLOOM
};

/* Path pack kinds */
enum {
	RPACK_BTC_STD = 0,   /* 44/49/84/86 + change */
	RPACK_ETH,
	RPACK_ELECTRUM,
	RPACK_CUSTOM,
	RPACK_ACCOUNT_SWEEP
};

typedef struct {
	int submode;                 /* RSUB_* */
	int path_pack;               /* RPACK_* */
	int include_change;          /* /1/N */
	int include_bip86;
	int account_max;             /* for account-sweep */
	int index_max;               /* -D */
	int eth_coin_type;           /* use 60' paths when set */
	int prism_langs;             /* ChecksumPrism: all languages */
	int shadow_bits;             /* Shadow160 / prefix-N bits */
	int handoff_bits;            /* HerdHandoff -H */
	int filter_strategy;         /* RFILTER_* */
	int checksum_first;          /* always on for recovery */
	char seed_mask[512];         /* "word ? ? ..." or full known seed */
	char pass_mask[128];         /* hashcat-style ?l?d… */
	char pass_file[512];
	char pass_rules_file[512];
	char model_file[512];
	char path_pack_file[512];
	char dual_target_file[512];
	char density_map_file[512];
	char funded_file[512];
	uint64_t milksad_t0;
	uint64_t milksad_t1;
	uint32_t mod_step;           /* residue M */
	uint32_t mod_rem;            /* residue R */
	int bsgs_strategy;           /* index into bsgs_modes[] (0..20); research >4 kept */
	char bsgs_name[32];          /* "grumpy", "orbit", … */
} ResearchConfig;

extern ResearchConfig g_research;

int research_parse_submode(const char *name);
int research_parse_path_pack(const char *name);
int research_parse_filter(const char *name);

/* Consume long options before getopt. Returns count consumed conceptually; mutates argc/argv. */
int research_consume_long_flags(int *argc, char **argv);

/* Wordlist helpers (english BIP-39 indices 0..2047) */
int research_word_index(const char *word, char **wordlist, int nwords);
int research_valid_last_word_count(int word_count); /* 128 for 12, 256 for 24, etc. */

/*
 * Enumerate next mnemonic candidate for mask/lastword modes.
 * state is opaque uint64_t cursor (start 0). Returns 1 if filled mnemonic_out, 0 if exhausted.
 * Requires known mask in g_research.seed_mask with ? placeholders.
 */
int research_next_mask_mnemonic(uint64_t *state, char *mnemonic_out, size_t out_sz,
                                char **wordlist, int wordlist_size,
                                int (*validate_fn)(const char *));

/* Build passphrase candidate from dict line + optional mask (simple digit/alpha expand). */
int research_pass_from_dict_line(const char *line, char *pass_out, size_t out_sz);

/* Milk Sad: MT19937 seeded with unix time -> 32-byte entropy as privkey-like bytes */
void research_milksad_key(uint32_t seed_time, uint8_t out32[32]);

/* Hilbert / Sobol map u in [0,1) into range — fill result bytes as big-endian 32-byte key offset */
void research_hilbert_u(uint64_t step, double *u_out);
void research_sobol_u(uint64_t step, int dim, double *u_out);

/* Prefix match for Shadow160 / prefix-N: compare first bits of two 20-byte hashes */
int research_hash_prefix_equal(const uint8_t *a, const uint8_t *b, int bits);

void research_print_banner(void);

/* Electrum v2: HMAC-SHA512("Seed version", norm) version byte + electrum PBKDF2 */
int research_electrum_normalize(const char *in, char *out, size_t out_sz);
int research_electrum_v2_version_ok(const char *mnemonic); /* 1 if standard seed version */
void research_electrum_to_seed(const char *mnemonic, const char *passphrase, uint8_t seed[64]);

/* Hashcat-lite passphrase mask expand: fills next candidate; state starts 0; returns 0 when done */
int research_pass_mask_next(const char *mask, uint64_t *state, char *pass_out, size_t out_sz);

/* Apply built-in + optional rules-file transforms to a base passphrase.
 * rule_index 0..N-1; returns 1 if pass_out filled, 0 if exhausted. */
int research_pass_rule_apply(const char *base, uint64_t rule_index,
                             const char *rules_file, char *pass_out, size_t out_sz);

/* RFC-1751: encode 8 bytes -> 6 English words into out (space-separated). */
int research_rfc1751_encode(const uint8_t key8[8], char *out, size_t out_sz);

/* BIP-85 style: derive 16/32-byte entropy from master seed + index (HMAC-SHA512 path tag). */
void research_bip85_entropy(const uint8_t master_seed[64], uint32_t index,
                            int word_count, uint8_t ent_out[32], int *ent_bytes);

/* FuseCascade key extractors from 20-byte hash160 */
uint64_t research_hash_key48(const uint8_t *h20);  /* first 6 bytes */
uint64_t research_hash_key96(const uint8_t *h20);  /* first 12 bytes as mixed 64 */
uint64_t research_hash_key160(const uint8_t *h20); /* first 8 bytes (default fuse) */

/* Orbit / negation map: fold secp256k1 x into canonical representative (min of x, p-x) */
void research_orbit_normalize_x(uint8_t x32[32]);

/*
 * Recovery enum for typo / permute / anagram / lattice over a known seed in seed_mask.
 * typo: single-word substitutions (checksum-first)
 * permute/anagram: factorial permutations of the known words
 * lattice: same as mask free-slots but prefers checksum-valid streams
 * Returns 1 if mnemonic_out filled, 0 if exhausted.
 */
int research_next_recovery_mnemonic(uint64_t *state, char *mnemonic_out, size_t out_sz,
                                    char **wordlist, int wordlist_size,
                                    int (*validate_fn)(const char *));

#ifdef __cplusplus
}

/* C++ path-pack descriptor */
struct ResearchPath {
	uint32_t indices[8];
	int len;
	int addr_type; /* 0 P2PKH, 1 P2SH-P2WPKH, 2 P2WPKH, 3 P2TR, 4 ETH */
	const char *name;
};

int research_build_path_pack(ResearchPath *out, int max_out, int pack, int index_max,
                             int include_change, int include_bip86, int eth);

#endif /* __cplusplus */

#endif
