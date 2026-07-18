/* TrueCollider research implants — extra engines (safe separate TU). */
#include "research_engine.h"
#include "hash/sha256.h"

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

int research_dual_target_load(void) {
	g_research.dual_loaded = 0;
	if(!g_research.dual_target_file[0]) return 0;
	FILE *fp = fopen(g_research.dual_target_file, "r");
	if(!fp) { fprintf(stderr, "[E] dual-target open failed\n"); return 0; }
	char line[128];
	while(fgets(line, sizeof(line), fp)) {
		char *p = line;
		while(*p == ' ' || *p == '\t') p++;
		if(*p == '#' || !*p) continue;
		size_t L = strlen(p);
		while(L && (p[L - 1] == '\n' || p[L - 1] == '\r')) p[--L] = 0;
		if(p[0] == '0' && (p[1] == 'x' || p[1] == 'X') && L >= 42) {
			for(int i = 0; i < 20; i++) {
				int a = hex_nibble(p[2 + i * 2]), b = hex_nibble(p[3 + i * 2]);
				if(a < 0 || b < 0) { fclose(fp); return 0; }
				g_research.dual_hash[i] = (uint8_t)((a << 4) | b);
			}
			g_research.dual_is_eth = 1;
			g_research.dual_loaded = 1;
			break;
		}
		if(L == 40) {
			for(int i = 0; i < 20; i++) {
				int a = hex_nibble(p[i * 2]), b = hex_nibble(p[i * 2 + 1]);
				if(a < 0 || b < 0) { fclose(fp); return 0; }
				g_research.dual_hash[i] = (uint8_t)((a << 4) | b);
			}
			g_research.dual_is_eth = 0;
			g_research.dual_loaded = 1;
			break;
		}
	}
	fclose(fp);
	if(g_research.dual_loaded)
		printf("[+] DualTarget loaded (%s)\n", g_research.dual_is_eth ? "ETH" : "hash160");
	return g_research.dual_loaded;
}

int research_dual_target_hit(const uint8_t *h20, int is_eth) {
	if(!g_research.dual_loaded || !h20) return 1;
	if(is_eth != g_research.dual_is_eth) return 0;
	return memcmp(h20, g_research.dual_hash, 20) == 0;
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

#endif /* __cplusplus */
