/* TrueCollider research implants — extra engines (safe separate TU). */
#include "research_engine.h"
#include "hash/sha256.h"
#include "base58/libbase58.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define strtok_r strtok_s
#endif

int research_mnemonic_from_entropy(const uint8_t *entropy, int ent_bytes,
                                   char **wordlist, int wordlist_size,
                                   char *out, size_t out_sz) {
	if(!entropy || !wordlist || wordlist_size != 2048 || !out || out_sz < 16) return 0;
	if(ent_bytes != 16 && ent_bytes != 20 && ent_bytes != 24 && ent_bytes != 28 && ent_bytes != 32)
		return 0;
	int strength = ent_bytes * 8;
	int checksum_bits = strength / 32;
	int word_count = (strength + checksum_bits) / 11;
	uint8_t hash[32];
	sha256((uint8_t *)entropy, (size_t)ent_bytes, hash);
	char bits[288];
	int bit_idx = 0;
	for(int i = 0; i < ent_bytes; i++)
		for(int b = 7; b >= 0; b--)
			bits[bit_idx++] = (char)((entropy[i] >> b) & 1);
	for(int i = 0; i < checksum_bits; i++)
		bits[bit_idx++] = (char)((hash[i / 8] >> (7 - (i % 8))) & 1);
	out[0] = 0;
	for(int i = 0; i < word_count; i++) {
		int idx = 0;
		for(int b = 0; b < 11; b++) idx = (idx << 1) | bits[i * 11 + b];
		if(idx < 0 || idx >= wordlist_size) { out[0] = 0; return 0; }
		if(i) strncat(out, " ", out_sz - strlen(out) - 1);
		strncat(out, wordlist[idx], out_sz - strlen(out) - 1);
	}
	return 1;
}

void research_weakrng_key(int submode, uint64_t cursor, uint8_t out32[32]) {
	memset(out32, 0, 32);
	if(submode == RSUB_MILKSAD) {
		uint32_t t = (uint32_t)(cursor & 0xffffffffu);
		if(g_research.milksad_t0)
			t = (uint32_t)((g_research.milksad_t0 + cursor) & 0xffffffffu);
		research_milksad_key(t, out32);
		return;
	}
	if(submode == RSUB_PROFANITY) {
		uint32_t seed = (uint32_t)(cursor & 0xffffffffu);
		out32[0] = (uint8_t)((seed >> 24) & 0xff);
		out32[1] = (uint8_t)((seed >> 16) & 0xff);
		out32[2] = (uint8_t)((seed >> 8) & 0xff);
		out32[3] = (uint8_t)(seed & 0xff);
		return;
	}
	if(submode == RSUB_ANDROID_SR || submode == RSUB_RANDSTORM) {
		uint8_t msg[8];
		memset(msg, 0, sizeof(msg));
		msg[0] = (uint8_t)(cursor & 0xff);
		msg[1] = (uint8_t)((cursor >> 8) & 0xff);
		msg[2] = (uint8_t)((cursor >> 16) & 0xff);
		msg[3] = (uint8_t)((cursor >> 24) & 0xff);
		msg[4] = (uint8_t)(submode == RSUB_ANDROID_SR ? 'A' : 'R');
		sha256(msg, 8, out32);
		return;
	}
	if(submode == RSUB_TIMESTAMP_KEY) {
		uint64_t ts = g_research.milksad_t0 ? (g_research.milksad_t0 + cursor) : cursor;
		uint8_t msg[8];
		for(int i = 0; i < 8; i++) msg[7 - i] = (uint8_t)((ts >> (8 * i)) & 0xff);
		sha256(msg, 8, out32);
		return;
	}
	research_milksad_key((uint32_t)(cursor & 0xffffffffu), out32);
}

int research_hex_mask_next(uint64_t *state, uint8_t out32[32]) {
	const char *tmpl = g_research.key_mask[0] ? g_research.key_mask :
	                   (g_research.pass_mask[0] ? g_research.pass_mask : g_research.seed_mask);
	if(!tmpl || !tmpl[0] || !state || !out32) return 0;
	char hex[96];
	int n = 0;
	for(const char *p = tmpl; *p && n < 64; p++) {
		char c = *p;
		if(c == ' ' || c == '\t') continue;
		if(c == '0' && (p[1] == 'x' || p[1] == 'X')) { p++; continue; }
		if(c == '?') { hex[n++] = '?'; continue; }
		if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			hex[n++] = (char)tolower((unsigned char)c);
	}
	hex[n] = 0;
	if(n != 64) {
		if(n > 64 || n < 1) return 0;
		char padded[65];
		memset(padded, '0', 64);
		memcpy(padded + (64 - n), hex, (size_t)n);
		padded[64] = 0;
		memcpy(hex, padded, 65);
		n = 64;
	}
	int free_n = 0;
	int free_pos[64];
	for(int i = 0; i < 64; i++)
		if(hex[i] == '?') free_pos[free_n++] = i;
	if(free_n > 16) free_n = 16;
	uint64_t space = (free_n == 0) ? 1ULL : (free_n >= 16 ? ~0ULL : (1ULL << (4 * free_n)));
	if(free_n > 0 && *state >= space) return 0;
	uint64_t s = (*state)++;
	char filled[65];
	memcpy(filled, hex, 65);
	static const char *H = "0123456789abcdef";
	for(int i = free_n - 1; i >= 0; i--) {
		filled[free_pos[i]] = H[s & 0xF];
		s >>= 4;
	}
	for(int i = 0; i < 32; i++) {
		char a = filled[i * 2], b = filled[i * 2 + 1];
		int va = (a <= '9') ? a - '0' : a - 'a' + 10;
		int vb = (b <= '9') ? b - '0' : b - 'a' + 10;
		out32[i] = (uint8_t)((va << 4) | vb);
	}
	return 1;
}

int research_load_density_map(const char *path) {
	if(!path || !path[0]) return 0;
	FILE *fp = fopen(path, "r");
	if(!fp) { fprintf(stderr, "[E] density-map open failed: %s\n", path); return 0; }
	double *w = NULL;
	int cap = 0, n = 0;
	char line[128];
	while(fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || *p == '\n' || !*p) continue;
		double v = strtod(p, NULL);
		if(v < 0) v = 0;
		if(n >= cap) {
			cap = cap ? cap * 2 : 1024;
			double *nw = (double *)realloc(w, (size_t)cap * sizeof(double));
			if(!nw) { free(w); fclose(fp); return 0; }
			w = nw;
		}
		w[n++] = v;
	}
	fclose(fp);
	if(n < 1) { free(w); return 0; }
	double sum = 0;
	for(int i = 0; i < n; i++) sum += w[i];
	if(sum <= 0) { free(w); return 0; }
	if(g_research.density_cdf) free(g_research.density_cdf);
	g_research.density_cdf = (double *)malloc((size_t)n * sizeof(double));
	if(!g_research.density_cdf) { free(w); return 0; }
	double run = 0;
	for(int i = 0; i < n; i++) {
		run += w[i] / sum;
		g_research.density_cdf[i] = run;
	}
	g_research.density_count = n;
	free(w);
	printf("[+] Density map: %d bins from %s\n", n, path);
	return 1;
}

double research_density_sample_u(uint64_t step) {
	if(!g_research.density_cdf || g_research.density_count < 1) {
		double u = 0;
		research_sobol_u(step, 0, &u);
		return u;
	}
	double probe = 0;
	research_sobol_u(step, 0, &probe);
	int lo = 0, hi = g_research.density_count - 1;
	while(lo < hi) {
		int mid = (lo + hi) / 2;
		if(g_research.density_cdf[mid] < probe) lo = mid + 1;
		else hi = mid;
	}
	return ((double)lo + 0.5) / (double)g_research.density_count;
}

static int hex_nibble(char c) {
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static int parse_h20_line(const char *p, uint8_t out[20], int *is_eth) {
	size_t L = strlen(p);
	const char *hex = p;
	*is_eth = 0;
	if(strncmp(p, "btc:", 4) == 0 || strncmp(p, "BTC:", 4) == 0) { hex = p + 4; *is_eth = 0; }
	else if(strncmp(p, "eth:", 4) == 0 || strncmp(p, "ETH:", 4) == 0) { hex = p + 4; *is_eth = 1; }
	while(*hex == ' ') hex++;
	L = strlen(hex);
	if(hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) { hex += 2; L -= 2; *is_eth = 1; }
	if(L < 40) return 0;
	for(int i = 0; i < 20; i++) {
		int a = hex_nibble(hex[i * 2]), b = hex_nibble(hex[i * 2 + 1]);
		if(a < 0 || b < 0) return 0;
		out[i] = (uint8_t)((a << 4) | b);
	}
	return 1;
}

int research_dual_target_load(void) {
	g_research.dual_loaded = 0;
	g_research.dual_has_btc = 0;
	g_research.dual_has_eth = 0;
	if(!g_research.dual_target_file[0]) return 0;
	FILE *fp = fopen(g_research.dual_target_file, "r");
	if(!fp) { fprintf(stderr, "[E] dual-target open failed\n"); return 0; }
	char line[160];
	while(fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || !*p) continue;
		size_t L = strlen(p);
		while(L && (p[L - 1] == '\n' || p[L - 1] == '\r')) p[--L] = 0;
		uint8_t h[20];
		int is_eth = 0;
		if(!parse_h20_line(p, h, &is_eth)) continue;
		if(is_eth) {
			memcpy(g_research.dual_eth, h, 20);
			g_research.dual_has_eth = 1;
			memcpy(g_research.dual_hash, h, 20);
			g_research.dual_is_eth = 1;
		} else {
			memcpy(g_research.dual_btc, h, 20);
			g_research.dual_has_btc = 1;
			memcpy(g_research.dual_hash, h, 20);
			g_research.dual_is_eth = 0;
		}
		g_research.dual_loaded = 1;
		if(g_research.dual_has_btc && g_research.dual_has_eth) break;
	}
	fclose(fp);
	if(g_research.dual_loaded)
		printf("[+] DualTarget loaded (btc=%d eth=%d)\n",
		       g_research.dual_has_btc, g_research.dual_has_eth);
	return g_research.dual_loaded;
}

int research_dual_target_hit(const uint8_t *h20, int is_eth) {
	if(!g_research.dual_loaded || !h20) return 1;
	if(g_research.dual_has_btc && g_research.dual_has_eth) {
		/* Pair mode: this side must match its hash; other side checked by caller. */
		if(is_eth) return memcmp(h20, g_research.dual_eth, 20) == 0;
		return memcmp(h20, g_research.dual_btc, 20) == 0;
	}
	if(is_eth) {
		if(!g_research.dual_has_eth && g_research.dual_is_eth)
			return memcmp(h20, g_research.dual_hash, 20) == 0;
		if(g_research.dual_has_eth) return memcmp(h20, g_research.dual_eth, 20) == 0;
		return 0;
	}
	if(g_research.dual_has_btc) return memcmp(h20, g_research.dual_btc, 20) == 0;
	if(!g_research.dual_is_eth) return memcmp(h20, g_research.dual_hash, 20) == 0;
	return 0;
}

int research_dual_needs_eth(void) {
	return g_research.dual_has_btc && g_research.dual_has_eth;
}
int research_dual_needs_btc(void) {
	return g_research.dual_has_btc && g_research.dual_has_eth;
}
int research_dual_eth_match(const uint8_t *h20) {
	if(!g_research.dual_has_eth || !h20) return 1;
	return memcmp(h20, g_research.dual_eth, 20) == 0;
}
int research_dual_btc_match(const uint8_t *h20) {
	if(!g_research.dual_has_btc || !h20) return 1;
	return memcmp(h20, g_research.dual_btc, 20) == 0;
}

int research_prepare_model_mask(void) {
	if(!g_research.model_file[0]) return 0;
	FILE *fp = fopen(g_research.model_file, "r");
	if(!fp) return 0;
	char line[512];
	char built[512];
	built[0] = 0;
	int slots = 0;
	while(fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || !*p) continue;
		size_t L = strlen(p);
		while(L && (p[L - 1] == '\n' || p[L - 1] == '\r')) p[--L] = 0;
		if(strchr(p, ' ') && !strchr(p, ':')) {
			strncpy(g_research.seed_mask, p, sizeof(g_research.seed_mask) - 1);
			g_research.seed_mask[sizeof(g_research.seed_mask) - 1] = 0;
			fclose(fp);
			printf("[+] Model→seed mask loaded (%zu chars)\n", strlen(g_research.seed_mask));
			return 1;
		}
		char *colon = strchr(p, ':');
		if(!colon) continue;
		*colon = 0;
		char *cands = colon + 1;
		char *bar = strchr(cands, '|');
		if(bar) *bar = 0;
		while(*cands == ' ') cands++;
		if(slots) strncat(built, " ", sizeof(built) - strlen(built) - 1);
		if(!cands[0] || strcmp(cands, "?") == 0 || strcmp(cands, "*") == 0)
			strncat(built, "?", sizeof(built) - strlen(built) - 1);
		else
			strncat(built, cands, sizeof(built) - strlen(built) - 1);
		slots++;
	}
	fclose(fp);
	if(slots >= 12) {
		strncpy(g_research.seed_mask, built, sizeof(g_research.seed_mask) - 1);
		g_research.seed_mask[sizeof(g_research.seed_mask) - 1] = 0;
		printf("[+] Model file → %d-slot seed mask\n", slots);
		return 1;
	}
	return 0;
}

int research_prepare_prefix_word_mask(char **wordlist, int wordlist_size) {
	if(!g_research.seed_mask[0] || !wordlist) return 0;
	char tmp[512];
	strncpy(tmp, g_research.seed_mask, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	char words[24][32];
	int nw = 0, changed = 0;
	char *sv = NULL;
	char *tok = strtok_r(tmp, " \t", &sv);
	while(tok && nw < 24) {
		strncpy(words[nw], tok, 31);
		words[nw][31] = 0;
		size_t L = strlen(words[nw]);
		if(L > 1 && words[nw][L - 1] == '*') {
			words[nw][L - 1] = 0;
			int hits = 0;
			for(int j = 0; j < wordlist_size; j++)
				if(wordlist[j] && strncmp(wordlist[j], words[nw], L - 1) == 0) hits++;
			if(hits > 0) {
				strncpy(words[nw], "?", 31);
				changed = 1;
			}
		}
		nw++;
		tok = strtok_r(NULL, " \t", &sv);
	}
	if(!changed) return 0;
	g_research.seed_mask[0] = 0;
	for(int i = 0; i < nw; i++) {
		if(i) strncat(g_research.seed_mask, " ", sizeof(g_research.seed_mask) - strlen(g_research.seed_mask) - 1);
		strncat(g_research.seed_mask, words[i], sizeof(g_research.seed_mask) - strlen(g_research.seed_mask) - 1);
	}
	printf("[+] Prefix-word expanded → %s\n", g_research.seed_mask);
	return 1;
}

int research_guess_language(char ***wordlists, const int *sizes, int nlangs) {
	if(!g_research.seed_mask[0] || !wordlists || nlangs < 1) return -1;
	char tmp[512];
	strncpy(tmp, g_research.seed_mask, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = 0;
	char words[24][32];
	int nw = 0;
	char *sv = NULL;
	char *tok = strtok_r(tmp, " \t", &sv);
	while(tok && nw < 24) {
		strncpy(words[nw], tok, 31);
		words[nw][31] = 0;
		nw++;
		tok = strtok_r(NULL, " \t", &sv);
	}
	int best = -1, best_hits = 0;
	for(int lang = 0; lang < nlangs; lang++) {
		if(!wordlists[lang] || !sizes || sizes[lang] != 2048) continue;
		int hits = 0;
		for(int w = 0; w < nw; w++) {
			if(words[w][0] == '?' || words[w][0] == 'x' || words[w][0] == 'X') continue;
			if(research_word_index(words[w], wordlists[lang], 2048) >= 0) hits++;
		}
		if(hits > best_hits) { best_hits = hits; best = lang; }
	}
	if(best >= 0)
		printf("[+] Language-guess: best list #%d (%d hits)\n", best, best_hits);
	return best;
}

int research_apply_language_guess(char ***wordlists, const int *sizes, int nlangs) {
	int best = research_guess_language(wordlists, sizes, nlangs);
	if(best < 0) return -1;
	g_research.language_pin = best;
	g_research.prism_langs = 0;
	printf("[+] Language pinned to list #%d\n", best);
	return best;
}

int research_wif_mask_next(uint64_t *state, uint8_t out32[32]) {
	const char *tmpl = g_research.key_mask[0] ? g_research.key_mask : g_research.seed_mask;
	if(!tmpl || !tmpl[0] || !state || !out32) return 0;
	static const char *B58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	char pat[80];
	int n = 0;
	for(const char *p = tmpl; *p && n < 75; p++) {
		if(*p == ' ' || *p == '\t') continue;
		pat[n++] = *p;
	}
	pat[n] = 0;
	if(n < 50) return 0;
	int free_n = 0, free_pos[8];
	for(int i = 0; i < n && free_n < 3; i++)
		if(pat[i] == '?') free_pos[free_n++] = i;
	uint64_t space = 1;
	for(int i = 0; i < free_n; i++) space *= 58ULL;
	if(free_n == 0) space = 1;
	/* Skip invalid checksums; bound attempts per call */
	for(int attempt = 0; attempt < 4096; attempt++) {
		if(*state >= space) return 0;
		uint64_t cur = (*state)++;
		char filled[80];
		memcpy(filled, pat, (size_t)n + 1);
		uint64_t s = cur;
		for(int i = free_n - 1; i >= 0; i--) {
			filled[free_pos[i]] = B58[s % 58ULL];
			s /= 58ULL;
		}
		uint8_t bin[64];
		memset(bin, 0, sizeof(bin));
		size_t cap = sizeof(bin);
		size_t binsz = cap;
		if(!b58tobin(bin, &binsz, filled, strlen(filled))) continue;
		if(binsz < 37) continue;
		uint8_t *payload = bin + (cap - binsz);
		uint8_t h1[32], h2[32];
		sha256(payload, binsz - 4, h1);
		sha256(h1, 32, h2);
		if(memcmp(h2, payload + (binsz - 4), 4) != 0) continue;
		if(payload[0] != 0x80) continue;
		memcpy(out32, payload + 1, 32);
		return 1;
	}
	return (*state < space) ? research_wif_mask_next(state, out32) : 0;
}

int research_pass_empty_plus_next(uint64_t *state, char *out, size_t out_sz) {
	static const char *defaults[] = {
		"", " ", "  ", "satoshi", "bitcoin", "Mnemonic", "mnemonic", "Password",
		"password", "123456", "12345678", "qwerty", "abc123", "letmein",
		"trustno1", "dragon", "master", "hello", "freedom", "whatever",
		"Blockchain", "crypto", "wallet", "seed", "TREZOR", "ledger",
		NULL
	};
	if(!state || !out || out_sz < 2) return 0;
	uint64_t i = *state;
	if(!defaults[i]) return 0;
	strncpy(out, defaults[i], out_sz - 1);
	out[out_sz - 1] = 0;
	(*state)++;
	return 1;
}

int research_pass_hybrid_combine(const char *base, const char *mask,
                                 uint64_t *state, char *out, size_t out_sz) {
	/* state: even = base+mask[k], odd = mask[k]+base; k = state/2 */
	if(!base || !mask || !mask[0] || !state || !out || out_sz < 2) return 0;
	uint64_t k = (*state) / 2ULL;
	int prefix = (int)((*state) % 2ULL);
	char frag[128];
	uint64_t mstate = k;
	if(!research_pass_mask_next(mask, &mstate, frag, sizeof(frag))) return 0;
	if(prefix) snprintf(out, out_sz, "%s%s", frag, base);
	else snprintf(out, out_sz, "%s%s", base, frag);
	(*state)++;
	return 1;
}

int research_pass_lattice_next(uint64_t *state, char *out, size_t out_sz) {
	if(!state || !out || out_sz < 8) return 0;
	static const char *pets[] = {
		"max","buddy","charlie","lucy","bella","luna","rocky","daisy","cooper","sadie", NULL
	};
	static const char *kb[] = {
		"qwerty","asdf","zxcv","1qaz","2wsx","qazwsx","password","iloveyou", NULL
	};
	uint64_t s = *state;
	/* 0..50: years 1980..2030 plain */
	if(s < 51) {
		snprintf(out, out_sz, "%d", 1980 + (int)s);
		(*state)++;
		return 1;
	}
	s -= 51;
	/* pets × years */
	int np = 0; while(pets[np]) np++;
	if(s < (uint64_t)np * 51ULL) {
		int pi = (int)(s / 51ULL);
		int yi = (int)(s % 51ULL);
		snprintf(out, out_sz, "%s%d", pets[pi], 1980 + yi);
		(*state)++;
		return 1;
	}
	s -= (uint64_t)np * 51ULL;
	int nk = 0; while(kb[nk]) nk++;
	if(s < (uint64_t)nk) {
		strncpy(out, kb[s], out_sz - 1);
		out[out_sz - 1] = 0;
		(*state)++;
		return 1;
	}
	s -= (uint64_t)nk;
	/* Capitalize + year */
	if(s < (uint64_t)np * 51ULL) {
		int pi = (int)(s / 51ULL);
		int yi = (int)(s % 51ULL);
		char name[32];
		strncpy(name, pets[pi], sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;
		if(name[0]) name[0] = (char)toupper((unsigned char)name[0]);
		snprintf(out, out_sz, "%s%d!", name, 1980 + yi);
		(*state)++;
		return 1;
	}
	return 0;
}

int research_mixed_script_normalize(char *inout, size_t inout_sz) {
	if(!inout || !inout_sz) return 0;
	/* ASCII fold: lowercase, strip combining-ish high bytes, map common Latin lookalikes */
	char tmp[512];
	size_t j = 0;
	for(size_t i = 0; inout[i] && j + 1 < sizeof(tmp) && j + 1 < inout_sz; i++) {
		unsigned char c = (unsigned char)inout[i];
		if(c >= 0x80) {
			/* drop non-ASCII; caller can also replace lookalikes */
			if(c == 0xC3 && inout[i + 1]) { /* crude UTF-8 Latin-1 supplement lead */
				unsigned char d = (unsigned char)inout[++i];
				/* àáâãäå → a, etc. rough map */
				if(d >= 0xA0 && d <= 0xA5) c = 'a';
				else if(d >= 0xA8 && d <= 0xAB) c = 'e';
				else if(d >= 0xAC && d <= 0xAF) c = 'i';
				else if(d >= 0xB2 && d <= 0xB6) c = 'o';
				else if(d >= 0xB9 && d <= 0xBC) c = 'u';
				else if(d == 0xB1) c = 'n';
				else continue;
			} else continue;
		}
		if(c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
		tmp[j++] = (char)c;
	}
	tmp[j] = 0;
	strncpy(inout, tmp, inout_sz - 1);
	inout[inout_sz - 1] = 0;
	return j > 0;
}

static int cmp_h20(const void *a, const void *b) {
	return memcmp(a, b, 20);
}

int research_funded_load(const char *path) {
	if(!path || !path[0]) return 0;
	FILE *fp = fopen(path, "r");
	if(!fp) { fprintf(stderr, "[E] funded file open failed: %s\n", path); return 0; }
	uint8_t *buf = NULL;
	int n = 0, cap = 0;
	char line[128];
	while(fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || !*p) continue;
		size_t L = strlen(p);
		while(L && (p[L - 1] == '\n' || p[L - 1] == '\r')) p[--L] = 0;
		uint8_t h[20];
		int is_eth = 0;
		if(!parse_h20_line(p, h, &is_eth)) continue;
		if(n >= cap) {
			cap = cap ? cap * 2 : 1024;
			uint8_t *nb = (uint8_t *)realloc(buf, (size_t)cap * 20);
			if(!nb) { free(buf); fclose(fp); return 0; }
			buf = nb;
		}
		memcpy(buf + (size_t)n * 20, h, 20);
		n++;
	}
	fclose(fp);
	if(n < 1) { free(buf); return 0; }
	qsort(buf, (size_t)n, 20, cmp_h20);
	if(g_research.funded_hashes) free(g_research.funded_hashes);
	g_research.funded_hashes = buf;
	g_research.funded_count = n;
	printf("[+] Funded filter: %d hash160 entries\n", n);
	return 1;
}

int research_funded_hit(const uint8_t *h20) {
	if(!g_research.funded_hashes || g_research.funded_count < 1 || !h20) return 1;
	return bsearch(h20, g_research.funded_hashes, (size_t)g_research.funded_count, 20, cmp_h20) != NULL;
}

#ifdef __cplusplus

int research_load_custom_path_file(ResearchPath *out, int max_out, const char *path) {
	FILE *fp = fopen(path, "r");
	if(!fp) {
		fprintf(stderr, "[E] Cannot open path pack file: %s\n", path);
		return 0;
	}
	int n = 0;
	char line[256];
	while(n < max_out && fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || *p == '\n' || *p == 0) continue;
		char *bar = strchr(p, '|');
		int atype = 0;
		if(bar) {
			*bar = 0;
			const char *tag = bar + 1;
			while(*tag == ' ') tag++;
			if(strncmp(tag, "p2wpkh", 6) == 0 || strncmp(tag, "84", 2) == 0) atype = 2;
			else if(strncmp(tag, "p2sh", 4) == 0 || strncmp(tag, "49", 2) == 0) atype = 1;
			else if(strncmp(tag, "p2tr", 4) == 0 || strncmp(tag, "86", 2) == 0) atype = 3;
			else if(strncmp(tag, "eth", 3) == 0) atype = 4;
		}
		for(char *q = p; *q; q++) {
			if(*q == '\n' || *q == '\r') { *q = 0; break; }
		}
		if(p[0] == 'm' && p[1] == '/') p += 2;
		uint32_t idx[8];
		int len = 0;
		char *sv = NULL;
		char *tok = strtok_r(p, "/", &sv);
		while(tok && len < 8) {
			int hardened = 0;
			size_t L = strlen(tok);
			if(L && (tok[L - 1] == '\'' || tok[L - 1] == 'h' || tok[L - 1] == 'H')) {
				hardened = 1;
				tok[L - 1] = 0;
			}
			unsigned long v = strtoul(tok, NULL, 10);
			idx[len++] = hardened ? (0x80000000u | (uint32_t)v) : (uint32_t)v;
			tok = strtok_r(NULL, "/", &sv);
		}
		if(len < 1) continue;
		out[n].len = len;
		out[n].addr_type = atype;
		out[n].name = "custom";
		for(int i = 0; i < 8; i++) out[n].indices[i] = (i < len) ? idx[i] : 0;
		n++;
	}
	fclose(fp);
	printf("[+] Loaded %d custom derivation paths from %s\n", n, path);
	return n;
}

int research_build_gap_limit_pack(ResearchPath *out, int max_out, int gap_limit,
                                  int include_bip86) {
	if(gap_limit < 1) gap_limit = 20;
	if(gap_limit > 200) gap_limit = 200;
	int n = 0;
	const struct { uint32_t purp; int atype; const char *name; int needs86; } rows[] = {
		{0x8000002C, 0, "BIP44-gap", 0},
		{0x80000031, 1, "BIP49-gap", 0},
		{0x80000054, 2, "BIP84-gap", 0},
		{0x80000056, 3, "BIP86-gap", 1},
	};
	for(size_t r = 0; r < sizeof(rows) / sizeof(rows[0]) && n < max_out; r++) {
		if(rows[r].needs86 && !include_bip86) continue;
		for(int ch = 0; ch <= 1 && n < max_out; ch++) {
			for(int i = 0; i <= gap_limit && n < max_out; i++) {
				out[n].indices[0] = rows[r].purp;
				out[n].indices[1] = 0x80000000;
				out[n].indices[2] = 0x80000000;
				out[n].indices[3] = (uint32_t)ch;
				out[n].indices[4] = (uint32_t)i;
				out[n].indices[5] = out[n].indices[6] = out[n].indices[7] = 0;
				out[n].len = 5;
				out[n].addr_type = rows[r].atype;
				out[n].name = rows[r].name;
				n++;
			}
		}
	}
	printf("[+] Gap-limit pack: %d paths (gap=%d)\n", n, gap_limit);
	return n;
}

int research_load_descriptor_file(ResearchPath *out, int max_out, const char *path) {
	/* Accept lines like: wpkh(.../0/*)  or  tr(.../1/*)  or raw m/84'/0'/0'/0/i|p2wpkh */
	FILE *fp = fopen(path, "r");
	if(!fp) {
		fprintf(stderr, "[E] Cannot open descriptor file: %s\n", path);
		return 0;
	}
	int n = 0;
	char line[512];
	while(n < max_out && fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || !*p) continue;
		for(char *q = p; *q; q++) if(*q == '\n' || *q == '\r') { *q = 0; break; }
		int atype = 2;
		int change = 0;
		int index_max = g_research.gap_limit > 0 ? g_research.gap_limit :
		                (g_research.index_max > 0 ? g_research.index_max : 20);
		if(strncmp(p, "pkh(", 4) == 0) atype = 0;
		else if(strncmp(p, "sh(wpkh(", 8) == 0 || strncmp(p, "sh(w", 4) == 0) atype = 1;
		else if(strncmp(p, "wpkh(", 5) == 0) atype = 2;
		else if(strncmp(p, "tr(", 3) == 0) atype = 3;
		else if(p[0] == 'm' || p[0] == 'M') {
			char *bar = strchr(p, '|');
			if(bar) {
				*bar = 0;
				const char *tag = bar + 1;
				if(strstr(tag, "p2tr") || strstr(tag, "86")) atype = 3;
				else if(strstr(tag, "p2wpkh") || strstr(tag, "84")) atype = 2;
				else if(strstr(tag, "p2sh") || strstr(tag, "49")) atype = 1;
			}
			if(p[0] == 'm' && p[1] == '/') p += 2;
			uint32_t idx[8]; int len = 0;
			char *sv = NULL;
			char buf[256];
			strncpy(buf, p, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
			char *tok = strtok_r(buf, "/", &sv);
			while(tok && len < 8) {
				int hardened = 0;
				size_t L = strlen(tok);
				if(L && (tok[L - 1] == '\'' || tok[L - 1] == 'h' || tok[L - 1] == 'H')) {
					hardened = 1; tok[L - 1] = 0;
				}
				if(strcmp(tok, "*") == 0 || strcmp(tok, "i") == 0) {
					int base_len = len;
					uint32_t base[8];
					memcpy(base, idx, sizeof(base));
					for(int i = 0; i <= index_max && n < max_out; i++) {
						memcpy(out[n].indices, base, sizeof(base));
						out[n].indices[base_len] = (uint32_t)i;
						out[n].len = base_len + 1;
						out[n].addr_type = atype;
						out[n].name = "descriptor";
						for(int z = out[n].len; z < 8; z++) out[n].indices[z] = 0;
						n++;
					}
					len = -1;
					break;
				}
				unsigned long v = strtoul(tok, NULL, 10);
				idx[len++] = hardened ? (0x80000000u | (uint32_t)v) : (uint32_t)v;
				tok = strtok_r(NULL, "/", &sv);
			}
			if(len > 0 && n < max_out) {
				for(int z = 0; z < 8; z++) out[n].indices[z] = (z < len) ? idx[z] : 0;
				out[n].len = len;
				out[n].addr_type = atype;
				out[n].name = "descriptor";
				n++;
			}
			continue;
		}
		/* descriptor with /0/* or /1/* */
		char *slash = strstr(p, "/0/*");
		if(slash) change = 0;
		else if((slash = strstr(p, "/1/*")) != NULL) change = 1;
		else if((slash = strstr(p, "/*")) != NULL) change = 0;
		uint32_t purp = (atype == 0) ? 0x8000002C :
		                (atype == 1) ? 0x80000031 :
		                (atype == 3) ? 0x80000056 : 0x80000054;
		for(int i = 0; i <= index_max && n < max_out; i++) {
			out[n].indices[0] = purp;
			out[n].indices[1] = 0x80000000;
			out[n].indices[2] = 0x80000000;
			out[n].indices[3] = (uint32_t)change;
			out[n].indices[4] = (uint32_t)i;
			out[n].indices[5] = out[n].indices[6] = out[n].indices[7] = 0;
			out[n].len = 5;
			out[n].addr_type = atype;
			out[n].name = "descriptor";
			n++;
		}
	}
	fclose(fp);
	printf("[+] Descriptor pack: %d paths from %s\n", n, path);
	return n;
}

#endif /* __cplusplus */
