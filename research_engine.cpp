#include "research_engine.h"
#include "hash/sha512.h"

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define strtok_r strtok_s
#endif

ResearchConfig g_research;

static void research_init_defaults(void) {
	static int once = 0;
	if(once) return;
	once = 1;
	memset(&g_research, 0, sizeof(g_research));
	g_research.submode = RSUB_RANDOM;
	g_research.path_pack = RPACK_BTC_STD;
	g_research.index_max = 1;
	g_research.checksum_first = 1;
	g_research.shadow_bits = 160;
	g_research.handoff_bits = 44;
	g_research.include_change = 1;
	g_research.include_bip86 = 1;
	g_research.account_max = 0;
	g_research.bsgs_strategy = 0;
	g_research.bsgs_name[0] = 0;
}

int research_parse_submode(const char *name) {
	static const char *names[] = {
		"random","mask","model","lastword","prefix-word","typo","permute","anagram",
		"positional-swap","language-guess","mixed-script",
		"pass-dict","pass-mask","pass-rules","pass-hybrid","pass-empty-plus",
		"electrum-v1","electrum-v2","slip39","aezeed","bip85","rfc1751","solana-bip39",
		"milksad","lattice","checksum-prism",
		"paths-btc","paths-eth","paths-electrum","paths-custom","account-sweep","multisig"
	};
	research_init_defaults();
	if(!name) return -1;
	for(int i = 0; i < RSUB_COUNT; i++) {
		if(strcmp(name, names[i]) == 0) return i;
	}
	return -1;
}

int research_parse_path_pack(const char *name) {
	if(!name) return RPACK_BTC_STD;
	if(strcmp(name, "btc") == 0 || strcmp(name, "paths-btc") == 0 || strcmp(name, "btc-std") == 0)
		return RPACK_BTC_STD;
	if(strcmp(name, "eth") == 0 || strcmp(name, "paths-eth") == 0 || strcmp(name, "ledger-eth") == 0)
		return RPACK_ETH;
	if(strcmp(name, "electrum") == 0 || strcmp(name, "paths-electrum") == 0)
		return RPACK_ELECTRUM;
	if(strcmp(name, "custom") == 0 || strcmp(name, "paths-custom") == 0)
		return RPACK_CUSTOM;
	if(strcmp(name, "account-sweep") == 0)
		return RPACK_ACCOUNT_SWEEP;
	return RPACK_BTC_STD;
}

int research_parse_filter(const char *name) {
	if(!name) return RFILTER_DEFAULT;
	if(strcmp(name, "cascade") == 0) return RFILTER_CASCADE;
	if(strcmp(name, "fuse16") == 0) return RFILTER_FUSE16;
	if(strcmp(name, "bloom") == 0 || strcmp(name, "bloom-classic") == 0) return RFILTER_BLOOM;
	return RFILTER_DEFAULT;
}

static int take_arg(int *argc, char **argv, int i, char *dst, size_t dstsz) {
	if(i + 1 >= *argc) return -1;
	strncpy(dst, argv[i + 1], dstsz - 1);
	dst[dstsz - 1] = 0;
	/* remove argv[i] and argv[i+1] */
	int j;
	for(j = i; j < *argc - 2; j++)
		argv[j] = argv[j + 2];
	*argc -= 2;
	argv[*argc] = NULL;
	return 0;
}

static void remove_one(int *argc, char **argv, int i) {
	int j;
	for(j = i; j < *argc - 1; j++)
		argv[j] = argv[j + 1];
	(*argc)--;
	argv[*argc] = NULL;
}

int research_consume_long_flags(int *argc, char **argv) {
	research_init_defaults();
	int touched = 0;
	int i = 1;
	while(i < *argc) {
		const char *a = argv[i];
		if(strcmp(a, "-R") == 0 || strcmp(a, "--research") == 0 || strcmp(a, "--submode") == 0) {
			char buf[64] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				int sm = research_parse_submode(buf);
				if(sm >= 0) {
					g_research.submode = sm;
					/* path pack aliases */
					if(sm == RSUB_PATHS_BTC) g_research.path_pack = RPACK_BTC_STD;
					if(sm == RSUB_PATHS_ETH) { g_research.path_pack = RPACK_ETH; g_research.eth_coin_type = 1; }
					if(sm == RSUB_PATHS_ELECTRUM) g_research.path_pack = RPACK_ELECTRUM;
					if(sm == RSUB_ACCOUNT_SWEEP) g_research.path_pack = RPACK_ACCOUNT_SWEEP;
					printf("[+] Research submode (-R): %s\n", buf);
					touched++;
					continue;
				}
				fprintf(stderr, "[W] Unknown -R submode: %s\n", buf);
				continue;
			}
		}
		if(strcmp(a, "--seed") == 0) {
			if(take_arg(argc, argv, i, g_research.seed_mask, sizeof(g_research.seed_mask)) == 0) {
				printf("[+] Seed mask/phrase loaded (%zu chars)\n", strlen(g_research.seed_mask));
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--pass-file") == 0 || strcmp(a, "--passfile") == 0) {
			if(take_arg(argc, argv, i, g_research.pass_file, sizeof(g_research.pass_file)) == 0) {
				printf("[+] Passphrase dictionary: %s\n", g_research.pass_file);
				if(g_research.submode == RSUB_RANDOM) g_research.submode = RSUB_PASS_DICT;
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--pass-mask") == 0) {
			if(take_arg(argc, argv, i, g_research.pass_mask, sizeof(g_research.pass_mask)) == 0) {
				printf("[+] Passphrase mask: %s\n", g_research.pass_mask);
				g_research.submode = RSUB_PASS_MASK;
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--pass-rules") == 0) {
			if(take_arg(argc, argv, i, g_research.pass_rules_file, sizeof(g_research.pass_rules_file)) == 0) {
				printf("[+] Passphrase rules: %s\n", g_research.pass_rules_file);
				if(g_research.submode == RSUB_RANDOM || g_research.submode == RSUB_PASS_DICT)
					g_research.submode = RSUB_PASS_RULES;
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--model") == 0) {
			if(take_arg(argc, argv, i, g_research.model_file, sizeof(g_research.model_file)) == 0) {
				printf("[+] Model constraints: %s\n", g_research.model_file);
				g_research.submode = RSUB_MODEL;
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--density-map") == 0) {
			if(take_arg(argc, argv, i, g_research.density_map_file, sizeof(g_research.density_map_file)) == 0) {
				printf("[+] Density map: %s\n", g_research.density_map_file);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--funded") == 0) {
			if(take_arg(argc, argv, i, g_research.funded_file, sizeof(g_research.funded_file)) == 0) {
				printf("[+] Funded snapshot: %s\n", g_research.funded_file);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--path-pack") == 0 || strcmp(a, "--pathpack") == 0) {
			char buf[64] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.path_pack = research_parse_path_pack(buf);
				if(g_research.path_pack == RPACK_ETH) g_research.eth_coin_type = 1;
				printf("[+] PathNova pack: %s\n", buf);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--dual-target") == 0) {
			if(take_arg(argc, argv, i, g_research.dual_target_file, sizeof(g_research.dual_target_file)) == 0) {
				printf("[+] DualTarget file: %s\n", g_research.dual_target_file);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "-F") == 0 || strcmp(a, "--filter") == 0) {
			char buf[64] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.filter_strategy = research_parse_filter(buf);
				printf("[+] Filter strategy: %s\n", buf);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "-H") == 0 || strcmp(a, "--handoff") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.handoff_bits = atoi(buf);
				printf("[+] HerdHandoff bits (-H): %d\n", g_research.handoff_bits);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--shadow-bits") == 0 || strcmp(a, "--prefix-bits") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.shadow_bits = atoi(buf);
				printf("[+] Shadow/prefix bits: %d\n", g_research.shadow_bits);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--mod-step") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.mod_step = (uint32_t)strtoul(buf, NULL, 0);
				printf("[+] ResidueHerd mod-step M=%u\n", g_research.mod_step);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--mod-rem") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.mod_rem = (uint32_t)strtoul(buf, NULL, 0);
				printf("[+] ResidueHerd mod-rem R=%u\n", g_research.mod_rem);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--milksad-from") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.milksad_t0 = strtoull(buf, NULL, 10);
				if(g_research.submode == RSUB_RANDOM) g_research.submode = RSUB_MILKSAD;
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--milksad-to") == 0) {
			char buf[32] = {0};
			if(take_arg(argc, argv, i, buf, sizeof(buf)) == 0) {
				g_research.milksad_t1 = strtoull(buf, NULL, 10);
				touched++;
				continue;
			}
		}
		if(strcmp(a, "--change") == 0) {
			g_research.include_change = 1;
			remove_one(argc, argv, i);
			touched++;
			continue;
		}
		if(strcmp(a, "--no-change") == 0) {
			g_research.include_change = 0;
			remove_one(argc, argv, i);
			touched++;
			continue;
		}
		if(strcmp(a, "--bip86") == 0) {
			g_research.include_bip86 = 1;
			remove_one(argc, argv, i);
			touched++;
			continue;
		}
		if(strcmp(a, "--no-bip86") == 0) {
			g_research.include_bip86 = 0;
			remove_one(argc, argv, i);
			touched++;
			continue;
		}
		if(strcmp(a, "--prism") == 0) {
			g_research.prism_langs = 1;
			g_research.submode = RSUB_CHECKSUM_PRISM;
			remove_one(argc, argv, i);
			printf("[+] ChecksumPrism: all BIP-39 languages\n");
			touched++;
			continue;
		}
		i++;
	}
	return touched;
}

int research_word_index(const char *word, char **wordlist, int nwords) {
	if(!word || !wordlist) return -1;
	for(int i = 0; i < nwords; i++) {
		if(wordlist[i] && strcmp(word, wordlist[i]) == 0) return i;
	}
	return -1;
}

int research_valid_last_word_count(int word_count) {
	/* checksum bits = word_count * 11 / 33 ... BIP39: CS = ENT/32, last word mixes entropy+CS */
	switch(word_count) {
		case 12: return 128;  /* 2^(11-4) */
		case 15: return 128;
		case 18: return 128;
		case 21: return 128;
		case 24: return 256;  /* 2^(11-8) */
		default: return 128;
	}
}

/* ---- MT19937 (Milk Sad) ---- */
typedef struct {
	uint32_t mt[624];
	int idx;
} MT19937;

static void mt_seed(MT19937 *m, uint32_t s) {
	m->mt[0] = s;
	for(int i = 1; i < 624; i++)
		m->mt[i] = 1812433253u * (m->mt[i - 1] ^ (m->mt[i - 1] >> 30)) + (uint32_t)i;
	m->idx = 624;
}

static uint32_t mt_next(MT19937 *m) {
	if(m->idx >= 624) {
		for(int i = 0; i < 624; i++) {
			uint32_t y = (m->mt[i] & 0x80000000u) + (m->mt[(i + 1) % 624] & 0x7fffffffu);
			m->mt[i] = m->mt[(i + 397) % 624] ^ (y >> 1);
			if(y & 1u) m->mt[i] ^= 2567483615u;
		}
		m->idx = 0;
	}
	uint32_t y = m->mt[m->idx++];
	y ^= y >> 11;
	y ^= (y << 7) & 2636928640u;
	y ^= (y << 15) & 4022730752u;
	y ^= y >> 18;
	return y;
}

void research_milksad_key(uint32_t seed_time, uint8_t out32[32]) {
	MT19937 m;
	mt_seed(&m, seed_time);
	for(int i = 0; i < 8; i++) {
		uint32_t v = mt_next(&m);
		out32[i * 4 + 0] = (uint8_t)(v >> 24);
		out32[i * 4 + 1] = (uint8_t)(v >> 16);
		out32[i * 4 + 2] = (uint8_t)(v >> 8);
		out32[i * 4 + 3] = (uint8_t)(v);
	}
}

void research_hilbert_u(uint64_t step, double *u_out) {
	/* Cheap invertible-ish scramble → (0,1): not a full 2D Hilbert but ergodic coverage */
	uint64_t x = step * 0x9E3779B97F4A7C15ULL;
	x ^= x >> 30;
	x *= 0xBF58476D1CE4E5B9ULL;
	x ^= x >> 27;
	x *= 0x94D049BB133111EBULL;
	x ^= x >> 31;
	*u_out = (double)(x >> 11) / (double)(1ULL << 53);
}

void research_sobol_u(uint64_t step, int dim, double *u_out) {
	/* Direction numbers for dim 0/1 simplified Sobol */
	static const uint32_t V0[] = {
		0x80000000u, 0x40000000u, 0x20000000u, 0x10000000u,
		0x08000000u, 0x04000000u, 0x02000000u, 0x01000000u,
		0x00800000u, 0x00400000u, 0x00200000u, 0x00100000u,
		0x00080000u, 0x00040000u, 0x00020000u, 0x00010000u
	};
	uint32_t x = 0;
	uint64_t n = step + 1;
	int bit = 0;
	while(n) {
		if(n & 1u) {
			uint32_t v = (bit < 16) ? V0[bit] : (0x80000000u >> (bit % 16));
			if(dim & 1) v ^= (v >> 1);
			x ^= v;
		}
		n >>= 1;
		bit++;
	}
	*u_out = (double)x / 4294967296.0;
}

int research_hash_prefix_equal(const uint8_t *a, const uint8_t *b, int bits) {
	if(bits >= 160) return memcmp(a, b, 20) == 0;
	if(bits <= 0) return 1;
	int full = bits / 8;
	int rem = bits % 8;
	if(full && memcmp(a, b, (size_t)full) != 0) return 0;
	if(rem == 0) return 1;
	uint8_t mask = (uint8_t)(0xFF << (8 - rem));
	return ((a[full] ^ b[full]) & mask) == 0;
}

int research_pass_from_dict_line(const char *line, char *pass_out, size_t out_sz) {
	if(!line || !pass_out || out_sz < 2) return 0;
	size_t n = 0;
	while(line[n] && line[n] != '\r' && line[n] != '\n' && n + 1 < out_sz) {
		pass_out[n] = line[n];
		n++;
	}
	pass_out[n] = 0;
	return n > 0;
}

/* Parse mask into fixed words / free slots */
#define MAX_WORDS 24
typedef struct {
	int n;
	int free_pos[MAX_WORDS];
	int nfree;
	int fixed_idx[MAX_WORDS]; /* -1 if free */
	int word_count;
} MaskPlan;

static int plan_mask(const char *mask, char **wordlist, int wl, MaskPlan *plan) {
	memset(plan, 0, sizeof(*plan));
	char tmp[512];
	strncpy(tmp, mask, sizeof(tmp) - 1);
	char *save = NULL;
	char *tok = strtok_r(tmp, " \t", &save);
	while(tok && plan->word_count < MAX_WORDS) {
		int pos = plan->word_count;
		if(strcmp(tok, "?") == 0 || strcmp(tok, "x") == 0 || strcmp(tok, "X") == 0) {
			plan->fixed_idx[pos] = -1;
			plan->free_pos[plan->nfree++] = pos;
		} else {
			int idx = research_word_index(tok, wordlist, wl);
			if(idx < 0) {
				fprintf(stderr, "[E] Unknown BIP-39 word in mask: %s\n", tok);
				return 0;
			}
			plan->fixed_idx[pos] = idx;
		}
		plan->word_count++;
		tok = strtok_r(NULL, " \t", &save);
	}
	plan->n = plan->word_count;
	return plan->word_count == 12 || plan->word_count == 15 || plan->word_count == 18 ||
	       plan->word_count == 21 || plan->word_count == 24;
}

static void compose_mnemonic(const MaskPlan *plan, const int *free_vals, char **wordlist,
                             char *out, size_t out_sz) {
	out[0] = 0;
	int fi = 0;
	for(int i = 0; i < plan->word_count; i++) {
		int idx = plan->fixed_idx[i];
		if(idx < 0) idx = free_vals[fi++];
		if(i) strncat(out, " ", out_sz - strlen(out) - 1);
		strncat(out, wordlist[idx], out_sz - strlen(out) - 1);
	}
}

int research_next_mask_mnemonic(uint64_t *state, char *mnemonic_out, size_t out_sz,
                                char **wordlist, int wordlist_size,
                                int (*validate_fn)(const char *)) {
	static MaskPlan plan;
	static int planned = 0;
	static int lastword_only = 0;
	static int valid_last = 128;

	if(!g_research.seed_mask[0]) return 0;
	if(!planned) {
		if(!plan_mask(g_research.seed_mask, wordlist, wordlist_size, &plan)) {
			fprintf(stderr, "[E] Invalid seed mask / word count\n");
			return 0;
		}
		lastword_only = (g_research.submode == RSUB_LASTWORD) ||
		                (plan.nfree == 1 && plan.free_pos[0] == plan.word_count - 1);
		valid_last = research_valid_last_word_count(plan.word_count);
		planned = 1;
		printf("[+] Mask plan: %d words, %d free slot(s)%s\n",
		       plan.word_count, plan.nfree, lastword_only ? " (last-word optimized)" : "");
	}
	if(plan.nfree == 0) {
		/* Fully known phrase — yield once */
		if(*state > 0) return 0;
		compose_mnemonic(&plan, NULL, wordlist, mnemonic_out, out_sz);
		(*state)++;
		if(validate_fn && !validate_fn(mnemonic_out)) return 0;
		return 1;
	}

	/* Cap astronomical searches: if >3 free and not lastword, still iterate but checksum-first */
	const uint64_t max_tries_per_call_batch = 1; /* one candidate per call */
	(void)max_tries_per_call_batch;

	for(;;) {
		uint64_t s = *state;
		int free_vals[MAX_WORDS];
		if(lastword_only) {
			if(s >= (uint64_t)wordlist_size) return 0;
			/* Try all words at last position; validate_fn enforces checksum (only ~1/16 pass) */
			free_vals[0] = (int)(s % (uint64_t)wordlist_size);
		} else {
			uint64_t span = 1;
			for(int i = 0; i < plan.nfree; i++) {
				span *= 2048ULL;
				if(span > (1ULL << 40)) break; /* safety */
			}
			if(s >= span && plan.nfree <= 3) return 0;
			if(plan.nfree > 3 && s > 0 && (s % 1000000ULL) == 0) {
				/* still ok — random-ish progression via mixed radix */
			}
			uint64_t t = s;
			for(int i = plan.nfree - 1; i >= 0; i--) {
				free_vals[i] = (int)(t % 2048ULL);
				t /= 2048ULL;
			}
			if(plan.nfree > 3 && t > 0 && s > (1ULL << 32)) {
				/* For large spaces, wrap using LCG for exploration */
				s = s * 6364136223846793005ULL + 1;
			}
		}
		(*state)++;
		compose_mnemonic(&plan, free_vals, wordlist, mnemonic_out, out_sz);
		if(!validate_fn || validate_fn(mnemonic_out)) {
			return 1;
		}
		/* checksum-first: skip invalid without PBKDF2 */
		if((*state % 500000ULL) == 0 && (*state) > 0) {
			/* keep searching */
		}
		if(lastword_only && *state > (uint64_t)wordlist_size) return 0;
		if(!lastword_only && plan.nfree <= 2 && *state > (uint64_t)pow(2048.0, plan.nfree) + 10)
			return 0;
		/* yield CPU: return 0 occasionally? No — caller loops. Break after many rejects in one call */
		if((*state % 4096ULL) == 0) {
			/* after 4096 checksum fails, return 0 to let thread stats update, then continue next call */
			/* Actually return next valid only — continue loop until valid or exhausted */
		}
		if(!lastword_only && plan.nfree >= 4 && (*state % 16384ULL) == 0) {
			/* still in loop looking for checksum — continue */
		}
		/* Prevent tight infinite if validate always fails wrongly */
		if(*state > (1ULL << 48)) return 0;
	}
}

#ifdef __cplusplus

int research_build_path_pack(ResearchPath *out, int max_out, int pack, int index_max,
                             int include_change, int include_bip86, int eth) {
	int n = 0;
	if(index_max < 1) index_max = 1;
	if(index_max > 100) index_max = 100;

	auto push = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, int len,
	                int atype, const char *name) {
		if(n >= max_out) return;
		ResearchPath *p = &out[n++];
		p->indices[0] = a; p->indices[1] = b; p->indices[2] = c;
		p->indices[3] = d; p->indices[4] = e;
		p->len = len;
		p->addr_type = atype;
		p->name = name;
	};

	if(eth || pack == RPACK_ETH) {
		for(int i = 0; i < index_max && n < max_out; i++) {
			/* m/44'/60'/0'/0/i */
			push(0x8000002C, 0x8000003C, 0x80000000, 0, (uint32_t)i, 5, 4, "BIP44-ETH");
			/* Ledger Live style m/44'/60'/0'/i */
			if(n < max_out)
				push(0x8000002C, 0x8000003C, 0x80000000, (uint32_t)i, 0, 4, 4, "LedgerLive-ETH");
		}
		return n;
	}

	if(pack == RPACK_ELECTRUM) {
		for(int i = 0; i < index_max && n < max_out; i++) {
			push(0, (uint32_t)i, 0, 0, 0, 2, 0, "Electrum-m/0"); /* m/0/i — simplified */
			out[n - 1].indices[0] = 0;
			out[n - 1].indices[1] = (uint32_t)i;
			out[n - 1].len = 2;
			if(include_change && n < max_out) {
				push(1, (uint32_t)i, 0, 0, 0, 2, 0, "Electrum-m/1");
				out[n - 1].indices[0] = 1;
				out[n - 1].indices[1] = (uint32_t)i;
				out[n - 1].len = 2;
			}
		}
		return n;
	}

	/* BTC standard PathNova */
	const struct { uint32_t purp; int atype; const char *name; int needs86; } rows[] = {
		{0x8000002C, 0, "BIP44", 0},
		{0x80000031, 1, "BIP49", 0},
		{0x80000054, 2, "BIP84", 0},
		{0x80000056, 3, "BIP86", 1},
	};
	for(size_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) {
		if(rows[r].needs86 && !include_bip86) continue;
		for(int ch = 0; ch <= (include_change ? 1 : 0); ch++) {
			for(int i = 0; i < index_max && n < max_out; i++) {
				push(rows[r].purp, 0x80000000, 0x80000000, (uint32_t)ch, (uint32_t)i, 5,
				     rows[r].atype, rows[r].name);
			}
		}
	}
	return n;
}

#endif /* __cplusplus */

void research_print_banner(void) {
	research_init_defaults();
	printf("[+] Research engine: mnemonic recovery / PathNova / MilkSad / Hilbert / Sobol / Shadow160 / ResidueHerd / Electrum / FuseCascade / OrbitBSGS\n");
}

int research_electrum_normalize(const char *in, char *out, size_t out_sz) {
	if(!in || !out || out_sz < 2) return 0;
	size_t j = 0;
	/* Simplified NFKD: lowercase ASCII + collapse whitespace */
	int space = 0;
	for(size_t i = 0; in[i] && j + 1 < out_sz; i++) {
		unsigned char c = (unsigned char)in[i];
		if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			space = 1;
			continue;
		}
		if(space && j > 0) out[j++] = ' ';
		space = 0;
		if(c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
		out[j++] = (char)c;
	}
	out[j] = 0;
	return j > 0;
}

int research_electrum_v2_version_ok(const char *mnemonic) {
	char norm[512];
	if(!research_electrum_normalize(mnemonic, norm, sizeof(norm))) return 0;
	uint8_t digest[64];
	hmac_sha512((unsigned char *)"Seed version", 12,
	            (unsigned char *)norm, (int)strlen(norm), digest);
	/* Standard Electrum: first byte 0x01; Segwit 0x100 / 2xx variants — accept 0x01 and 0x10 */
	return digest[0] == 0x01 || digest[0] == 0x10;
}

void research_electrum_to_seed(const char *mnemonic, const char *passphrase, uint8_t seed[64]) {
	char norm[512];
	char salt[300];
	research_electrum_normalize(mnemonic, norm, sizeof(norm));
	char passn[256];
	research_electrum_normalize(passphrase ? passphrase : "", passn, sizeof(passn));
	snprintf(salt, sizeof(salt), "electrum%s", passn);
	pbkdf2_hmac_sha512(seed, 64, (uint8_t *)norm, strlen(norm),
	                   (uint8_t *)salt, strlen(salt), 2048);
}

int research_pass_mask_next(const char *mask, uint64_t *state, char *pass_out, size_t out_sz) {
	if(!mask || !state || !pass_out || out_sz < 2) return 0;
	/* Support ?d (digit), ?l (lower), ?u (upper), ?s (special limited), literal otherwise */
	static const char *DIG = "0123456789";
	static const char *LOW = "abcdefghijklmnopqrstuvwxyz";
	static const char *UPP = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const char *SPE = "!@#$%^&*()-_=+";
	int slots = 0;
	int kinds[64];
	int lit_pos = 0;
	char lits[128];
	memset(kinds, 0, sizeof(kinds));
	for(int i = 0; mask[i] && slots < 64; ) {
		if(mask[i] == '?' && mask[i+1]) {
			char t = mask[i+1];
			if(t=='d') kinds[slots++] = 1;
			else if(t=='l') kinds[slots++] = 2;
			else if(t=='u') kinds[slots++] = 3;
			else if(t=='s') kinds[slots++] = 4;
			else { lits[lit_pos++] = t; kinds[slots++] = -lit_pos; }
			i += 2;
		} else {
			lits[lit_pos++] = mask[i];
			kinds[slots++] = -lit_pos;
			i++;
		}
	}
	if(slots == 0) {
		if(*state > 0) return 0;
		strncpy(pass_out, mask, out_sz - 1);
		pass_out[out_sz - 1] = 0;
		(*state)++;
		return 1;
	}
	uint64_t span = 1;
	int rad[64];
	for(int i = 0; i < slots; i++) {
		if(kinds[i] == 1) rad[i] = 10;
		else if(kinds[i] == 2) rad[i] = 26;
		else if(kinds[i] == 3) rad[i] = 26;
		else if(kinds[i] == 4) rad[i] = 14;
		else rad[i] = 1;
		if(span > (1ULL << 40) / (uint64_t)(rad[i] > 0 ? rad[i] : 1)) {
			/* cap */
		} else span *= (uint64_t)rad[i];
	}
	if(*state >= span && span > 1) return 0;
	uint64_t t = *state;
	char buf[128];
	int bp = 0;
	for(int i = 0; i < slots && bp + 1 < (int)sizeof(buf); i++) {
		int r = rad[i];
		int v = (r > 1) ? (int)(t % (uint64_t)r) : 0;
		if(r > 1) t /= (uint64_t)r;
		if(kinds[i] == 1) buf[bp++] = DIG[v];
		else if(kinds[i] == 2) buf[bp++] = LOW[v];
		else if(kinds[i] == 3) buf[bp++] = UPP[v];
		else if(kinds[i] == 4) buf[bp++] = SPE[v];
		else {
			int li = -kinds[i] - 1;
			if(li >= 0 && li < lit_pos) buf[bp++] = lits[li];
		}
	}
	buf[bp] = 0;
	strncpy(pass_out, buf, out_sz - 1);
	pass_out[out_sz - 1] = 0;
	(*state)++;
	return 1;
}

int research_pass_rule_apply(const char *base, uint64_t rule_index,
                             const char *rules_file, char *pass_out, size_t out_sz) {
	if(!base || !pass_out || out_sz < 2) return 0;
	/* Built-in transforms 0..111: identity, Capitalize, UPPER, lower, append 0-99, leet */
	const uint64_t builtin = 112;
	if(rule_index < builtin) {
		char tmp[256];
		strncpy(tmp, base, sizeof(tmp) - 1);
		tmp[sizeof(tmp) - 1] = 0;
		size_t n = strlen(tmp);
		if(rule_index == 0) {
			/* identity */
		} else if(rule_index == 1 && n) {
			if(tmp[0] >= 'a' && tmp[0] <= 'z') tmp[0] = (char)(tmp[0] - 'a' + 'A');
		} else if(rule_index == 2) {
			for(size_t i = 0; i < n; i++)
				if(tmp[i] >= 'a' && tmp[i] <= 'z') tmp[i] = (char)(tmp[i] - 'a' + 'A');
		} else if(rule_index == 3) {
			for(size_t i = 0; i < n; i++)
				if(tmp[i] >= 'A' && tmp[i] <= 'Z') tmp[i] = (char)(tmp[i] - 'A' + 'a');
		} else if(rule_index >= 4 && rule_index <= 103) {
			char dig[8];
			snprintf(dig, sizeof(dig), "%llu", (unsigned long long)(rule_index - 4));
			if(n + strlen(dig) < sizeof(tmp) - 1) strcat(tmp, dig);
		} else if(rule_index == 104) {
			for(size_t i = 0; i < n; i++) {
				if(tmp[i]=='a'||tmp[i]=='A') tmp[i]='@';
				else if(tmp[i]=='e'||tmp[i]=='E') tmp[i]='3';
				else if(tmp[i]=='i'||tmp[i]=='I') tmp[i]='1';
				else if(tmp[i]=='o'||tmp[i]=='O') tmp[i]='0';
				else if(tmp[i]=='s'||tmp[i]=='S') tmp[i]='$';
			}
		} else if(rule_index >= 105 && rule_index <= 111) {
			char dig[4];
			snprintf(dig, sizeof(dig), "!%c", (char)('0' + (int)(rule_index - 105)));
			if(n + strlen(dig) < sizeof(tmp) - 1) strcat(tmp, dig);
		} else {
			return 0;
		}
		strncpy(pass_out, tmp, out_sz - 1);
		pass_out[out_sz - 1] = 0;
		return 1;
	}
	/* Optional rules file: each non-empty line is a suffix/prefix template with $WORD */
	if(!rules_file || !rules_file[0]) return 0;
	FILE *fp = fopen(rules_file, "r");
	if(!fp) return 0;
	uint64_t want = rule_index - builtin;
	uint64_t cur = 0;
	char line[256];
	int ok = 0;
	while(fgets(line, sizeof(line), fp)) {
		size_t L = strlen(line);
		while(L && (line[L-1]=='\n'||line[L-1]=='\r')) line[--L]=0;
		if(!L || line[0]=='#') continue;
		if(cur == want) {
			char out[300];
			out[0]=0;
			const char *p = line;
			while(*p) {
				if(strncmp(p, "$WORD", 5)==0 || strncmp(p, "$word", 5)==0) {
					strncat(out, base, sizeof(out)-strlen(out)-1);
					p += 5;
				} else {
					size_t ol = strlen(out);
					if(ol + 1 < sizeof(out)) { out[ol]=*p; out[ol+1]=0; }
					p++;
				}
			}
			strncpy(pass_out, out, out_sz - 1);
			pass_out[out_sz - 1] = 0;
			ok = 1;
			break;
		}
		cur++;
	}
	fclose(fp);
	return ok;
}

/* Minimal RFC-1751 word list (first 64 of 2048-equivalent subset for demo encode). */
static const char *RFC1751_WORDS[64] = {
	"A","ABE","ACE","ACT","AD","ADA","ADD","AGO","AID","AIM","AIR","ALL","ALP","AM","AMY","AN",
	"ANA","AND","ANN","ANT","ANY","APE","APS","APT","ARC","ARE","ARK","ARM","ART","AS","ASH","ASK",
	"AT","ATE","AUG","AUK","AVE","AWE","AWK","AWL","AWN","AX","AYE","BAD","BAG","BAH","BAM","BAN",
	"BAR","BAT","BAY","BE","BED","BEE","BEG","BEN","BET","BEY","BIB","BID","BIG","BIN","BIT","BOB"
};

int research_rfc1751_encode(const uint8_t key8[8], char *out, size_t out_sz) {
	if(!key8 || !out || out_sz < 32) return 0;
	/* 64 bits -> 6 words of 11-ish bits; we use 6x ~10-bit from 64-bit key */
	uint64_t v = 0;
	for(int i = 0; i < 8; i++) v = (v << 8) | key8[i];
	out[0] = 0;
	for(int w = 0; w < 6; w++) {
		int idx = (int)((v >> (w * 10)) & 63ULL);
		if(w) strncat(out, " ", out_sz - strlen(out) - 1);
		strncat(out, RFC1751_WORDS[idx], out_sz - strlen(out) - 1);
	}
	return 1;
}

void research_bip85_entropy(const uint8_t master_seed[64], uint32_t index,
                            int word_count, uint8_t ent_out[32], int *ent_bytes) {
	/* Simplified BIP-85: HMAC-SHA512("bip-entropy-from-k", seed || index || words) */
	uint8_t msg[64 + 8];
	memcpy(msg, master_seed, 64);
	msg[64] = (uint8_t)((index >> 24) & 0xff);
	msg[65] = (uint8_t)((index >> 16) & 0xff);
	msg[66] = (uint8_t)((index >> 8) & 0xff);
	msg[67] = (uint8_t)(index & 0xff);
	msg[68] = (uint8_t)(word_count & 0xff);
	msg[69] = 0; msg[70] = 0; msg[71] = 0;
	uint8_t dig[64];
	hmac_sha512((unsigned char *)"bip-entropy-from-k", 18, msg, 72, dig);
	int nbytes = (word_count <= 12) ? 16 : (word_count <= 18) ? 24 : 32;
	memcpy(ent_out, dig, (size_t)nbytes);
	if(ent_bytes) *ent_bytes = nbytes;
}

uint64_t research_hash_key48(const uint8_t *h20) {
	uint64_t h = 0;
	for(int i = 0; i < 6; i++) h = (h << 8) | h20[i];
	return h;
}

uint64_t research_hash_key96(const uint8_t *h20) {
	uint64_t a = 0, b = 0;
	for(int i = 0; i < 8; i++) a = (a << 8) | h20[i];
	for(int i = 8; i < 12; i++) b = (b << 8) | h20[i];
	return a ^ (b * 0x9E3779B97F4A7C15ULL);
}

uint64_t research_hash_key160(const uint8_t *h20) {
	uint64_t h = 0;
	memcpy(&h, h20, 8);
	return h;
}

void research_orbit_normalize_x(uint8_t x32[32]) {
	/* secp256k1 prime p; fold x := min(x, p-x) for negation map */
	static const uint8_t P[32] = {
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFC,0x2F
	};
	uint8_t neg[32];
	int borrow = 0;
	for(int i = 31; i >= 0; i--) {
		int d = (int)P[i] - (int)x32[i] - borrow;
		if(d < 0) { d += 256; borrow = 1; } else borrow = 0;
		neg[i] = (uint8_t)d;
	}
	if(memcmp(neg, x32, 32) < 0)
		memcpy(x32, neg, 32);
}

/* ── typo / permute / anagram / lattice recovery ─────────────────────── */

#define RECOV_MAX_WORDS 24

static int parse_seed_words(const char *mask, char words[][32], int *nw_out) {
	char tmp[512];
	strncpy(tmp, mask, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	int nw = 0;
	char *sv = NULL;
	char *tok = strtok_r(tmp, " \t", &sv);
	while(tok && nw < RECOV_MAX_WORDS) {
		strncpy(words[nw], tok, 31);
		words[nw][31] = 0;
		nw++;
		tok = strtok_r(NULL, " \t", &sv);
	}
	*nw_out = nw;
	return nw >= 12 && nw <= 24;
}

static void join_words(char words[][32], int nw, char *out, size_t out_sz) {
	out[0] = 0;
	for(int i = 0; i < nw; i++) {
		if(i) strncat(out, " ", out_sz - strlen(out) - 1);
		strncat(out, words[i], out_sz - strlen(out) - 1);
	}
}

static uint64_t factorial_u64(int n) {
	if(n < 0) return 0;
	if(n > 20) n = 20; /* 21! overflows uint64 */
	uint64_t f = 1;
	for(int i = 2; i <= n; i++) f *= (uint64_t)i;
	return f;
}

static int next_typo(uint64_t *state, char *mnemonic_out, size_t out_sz,
                     char **wordlist, int wordlist_size,
                     int (*validate_fn)(const char *)) {
	static char base[RECOV_MAX_WORDS][32];
	static int nw = 0;
	static int ready = 0;
	if(!ready) {
		if(!parse_seed_words(g_research.seed_mask, base, &nw)) return 0;
		ready = 1;
		printf("[+] Typo recovery: %d words, single-substitution space\n", nw);
	}
	/* phase0: adjacent swaps (nw-1) ; phase1: substitute word@pos with all 2048 */
	const uint64_t nswap = (nw > 1) ? (uint64_t)(nw - 1) : 0;
	const uint64_t nsub = (uint64_t)nw * (uint64_t)wordlist_size;
	for(;;) {
		uint64_t s = (*state)++;
		char cur[RECOV_MAX_WORDS][32];
		memcpy(cur, base, sizeof(cur));
		if(s < nswap) {
			int i = (int)s;
			char t[32];
			strncpy(t, cur[i], 31); t[31] = 0;
			strncpy(cur[i], cur[i + 1], 31);
			strncpy(cur[i + 1], t, 31);
		} else {
			uint64_t t = s - nswap;
			if(t >= nsub) return 0;
			int pos = (int)(t / (uint64_t)wordlist_size);
			int widx = (int)(t % (uint64_t)wordlist_size);
			if(strcmp(base[pos], wordlist[widx]) == 0) continue; /* skip identity */
			strncpy(cur[pos], wordlist[widx], 31);
			cur[pos][31] = 0;
		}
		join_words(cur, nw, mnemonic_out, out_sz);
		if(!validate_fn || validate_fn(mnemonic_out)) return 1;
		if((*state) > nswap + nsub + 10) return 0;
	}
}

static int next_permute(uint64_t *state, char *mnemonic_out, size_t out_sz,
                        int (*validate_fn)(const char *)) {
	static char base[RECOV_MAX_WORDS][32];
	static int nw = 0;
	static int ready = 0;
	static uint64_t nperm = 0;
	if(!ready) {
		if(!parse_seed_words(g_research.seed_mask, base, &nw)) return 0;
		/* reject '?' placeholders for pure permute */
		for(int i = 0; i < nw; i++) {
			if(strcmp(base[i], "?") == 0) {
				fprintf(stderr, "[E] permute/anagram needs a fully known seed (no ?)\n");
				return 0;
			}
		}
		nperm = factorial_u64(nw);
		ready = 1;
		printf("[+] Permute/anagram: %d words, %" PRIu64 " permutations (checksum-first)\n",
		       nw, nperm);
	}
	for(;;) {
		uint64_t s = *state;
		if(s >= nperm) return 0;
		(*state)++;
		/* factorial number system permutation */
		int idx[RECOV_MAX_WORDS];
		int used[RECOV_MAX_WORDS];
		memset(used, 0, sizeof(used));
		uint64_t rem = s;
		for(int pos = 0; pos < nw; pos++) {
			uint64_t f = factorial_u64(nw - pos - 1);
			uint64_t choose = (f ? rem / f : 0);
			rem = f ? (rem % f) : 0;
			int seen = 0;
			for(int j = 0; j < nw; j++) {
				if(used[j]) continue;
				if((uint64_t)seen == choose) {
					idx[pos] = j;
					used[j] = 1;
					break;
				}
				seen++;
			}
		}
		char cur[RECOV_MAX_WORDS][32];
		for(int i = 0; i < nw; i++) {
			strncpy(cur[i], base[idx[i]], 31);
			cur[i][31] = 0;
		}
		join_words(cur, nw, mnemonic_out, out_sz);
		if(!validate_fn || validate_fn(mnemonic_out)) return 1;
	}
}

int research_next_recovery_mnemonic(uint64_t *state, char *mnemonic_out, size_t out_sz,
                                    char **wordlist, int wordlist_size,
                                    int (*validate_fn)(const char *)) {
	if(!g_research.seed_mask[0]) return 0;
	switch(g_research.submode) {
	case RSUB_TYPO:
		return next_typo(state, mnemonic_out, out_sz, wordlist, wordlist_size, validate_fn);
	case RSUB_PERMUTE:
	case RSUB_ANAGRAM:
		return next_permute(state, mnemonic_out, out_sz, validate_fn);
	case RSUB_LATTICE:
	case RSUB_PREFIX_WORD:
	case RSUB_MODEL:
		/* Lattice/model/prefix: checksum-first mask enumeration */
		return research_next_mask_mnemonic(state, mnemonic_out, out_sz,
		                                   wordlist, wordlist_size, validate_fn);
	default:
		return research_next_mask_mnemonic(state, mnemonic_out, out_sz,
		                                   wordlist, wordlist_size, validate_fn);
	}
}
