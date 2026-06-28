/*
Developed & Modified by TrueScent
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <inttypes.h>
#include "base58/libbase58.h"
#include "rmd160/rmd160.h"
#include "oldbloom/oldbloom.h"
#include "bloom/bloom.h"
#include "binaryfuse/binaryfuse_wrapper.h"
#include "sha3/sha3.h"
#include "util.h"

#include "secp256k1/SECP256k1.h"
#include "secp256k1/Point.h"
#include "secp256k1/Int.h"
#include "secp256k1/IntGroup.h"
#include "secp256k1/Random.h"

#include "hash/sha256.h"
#include "hash/sha512.h"
#include "hash/ripemd160.h"

#if defined(__MINGW32__) || defined(__MINGW64__)
static int rand_r(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed >> 16) & 0x7FFF;
}
#endif

#if defined(_WIN64) && !defined(__CYGWIN__)
#include <getopt.h>
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/random.h>
#endif

#ifdef __unix__
#ifdef __CYGWIN__
#else
#include <linux/random.h>
#endif
#endif

#define CRYPTO_NONE 0
#define CRYPTO_BTC 1
#define CRYPTO_ETH 2
#define CRYPTO_ALL 3
#define CRYPTO_TROOT 4
#define CRYPTO_BCH 5
#define CRYPTO_BTG 6
#define CRYPTO_ETC 7
#define CRYPTO_LTC 8
#define CRYPTO_DOGE 9
#define CRYPTO_XRP 10
#define CRYPTO_SOL 11

#define MODE_XPOINT 0
#define MODE_ADDRESS 1
#define MODE_BSGS 2
#define MODE_RMD160 3
#define MODE_PUB2RMD 4
#define MODE_MINIKEYS 5
#define MODE_VANITY 6
#define MODE_MNEMONIC 7
#define MODE_POETRY 8
#define MODE_BRAINWALLET 9
#define MODE_PUB2ADDR 10

#define SEARCH_UNCOMPRESS 0
#define SEARCH_COMPRESS 1
#define SEARCH_BOTH 2

#define SEARCHMODE_SEQUENTIAL 0
#define SEARCHMODE_RANDOM 1
#define SEARCHMODE_CHAOS 2
#define SEARCHMODE_GRAVITY 3
#define SEARCHMODE_SPIRAL 4
#define SEARCHMODE_REVERSE 5
#define SEARCHMODE_AUTO 6

uint32_t  THREADBPWORKLOAD = 1048576;

struct checksumsha256	{
	char data[32];
	char backup[32];
};

struct bsgs_xvalue	{
	uint8_t value[6];
	uint64_t index;
};

struct address_value	{
	uint8_t value[20];
};

struct troot_value	{
	uint8_t value[32];
};

struct tothread {
	int nt;     //Number thread
	char *rs;   //range start
	char *rpt;  //rng per thread
};

struct bPload	{
	uint32_t threadid;
	uint64_t from;
	uint64_t to;
	uint64_t counter;
	uint64_t workload;
	uint32_t aux;
	uint32_t finished;
};

#if defined(_WIN64) && !defined(__CYGWIN__) && !defined(__MINGW64__)
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
PACK(struct publickey
{
	uint8_t parity;
	union {
		uint8_t data8[32];
		uint32_t data32[8];
		uint64_t data64[4];
	} X;
});
#else
struct __attribute__((__packed__)) publickey {
  uint8_t parity;
	union	{
		uint8_t data8[32];
		uint32_t data32[8];
		uint64_t data64[4];
	} X;
};
#endif

const char *Ccoinbuffer_default = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

char *Ccoinbuffer = (char*) Ccoinbuffer_default;
char *str_baseminikey = NULL;
char *raw_baseminikey = NULL;
char *minikeyN = NULL;
int minikey_n_limit;
	
const char *version = "TrueCollider Search Modes + Binary Fuse Filters";

#define CPU_GRP_SIZE 1024

std::vector<Point> Gn;
Point _2Gn;

std::vector<Point> GSn;
Point _2GSn;

void menu();
void init_generator();

int searchbinary(struct address_value *buffer,char *data,int64_t array_length);
void sleep_ms(int milliseconds);

void _sort(struct address_value *arr,int64_t N);
void _insertionsort(struct address_value *arr, int64_t n);
void _introsort(struct address_value *arr,uint32_t depthLimit, int64_t n);
void _swap(struct address_value *a,struct address_value *b);
int64_t _partition(struct address_value *arr, int64_t n);
void _myheapsort(struct address_value	*arr, int64_t n);
void _heapify(struct address_value *arr, int64_t n, int64_t i);

void bsgs_sort(struct bsgs_xvalue *arr,int64_t n);
void bsgs_myheapsort(struct bsgs_xvalue *arr, int64_t n);
void bsgs_insertionsort(struct bsgs_xvalue *arr, int64_t n);
void bsgs_introsort(struct bsgs_xvalue *arr,uint32_t depthLimit, int64_t n);
void bsgs_swap(struct bsgs_xvalue *a,struct bsgs_xvalue *b);
void bsgs_heapify(struct bsgs_xvalue *arr, int64_t n, int64_t i);
int64_t bsgs_partition(struct bsgs_xvalue *arr, int64_t n);

int bsgs_searchbinary(struct bsgs_xvalue *arr,char *data,int64_t array_length,uint64_t *r_value);
int bsgs_secondcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey);
int bsgs_thirdcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey);

void sha256sse_22(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3);
void sha256sse_23(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3);

bool vanityrmdmatch(unsigned char *rmdhash);
void writevanitykey(bool compress,Int *key);
int addvanity(char *target);
int minimum_same_bytes(unsigned char* A,unsigned char* B, int length);

void writekey(bool compressed,Int *key);
void writekeyeth(Int *key);

void checkpointer(void *ptr,const char *file,const char *function,const  char *name,int line);

bool isBase58(char c);
bool isValidBase58String(char *str);

bool readFileAddress(char *fileName);
bool readFileVanity(char *fileName);
bool forceReadFileAddress(char *fileName);
bool forceReadFileAddressEth(char *fileName);
bool forceReadFileXPoint(char *fileName);
bool processOneVanity();

bool initBloomFilter(struct bloom *bloom_arg,uint64_t items_bloom);

int address_check(const void *buffer, int len);

void writeFileIfNeeded(const char *fileName);

void calcualteindex(int i,Int *key);
#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_vanity(LPVOID vargp);
DWORD WINAPI thread_process_minikeys(LPVOID vargp);
DWORD WINAPI thread_process(LPVOID vargp);
DWORD WINAPI thread_process_mnemonic(LPVOID vargp);
DWORD WINAPI thread_process_derived(LPVOID vargp);
DWORD WINAPI thread_process_troot(LPVOID vargp);
DWORD WINAPI thread_process_poetry(LPVOID vargp);
DWORD WINAPI thread_process_brainwallet(LPVOID vargp);
DWORD WINAPI thread_process_pub2addr(LPVOID vargp);
DWORD WINAPI thread_process_bsgs(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_backward(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_both(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_random(LPVOID vargp);
DWORD WINAPI thread_process_bsgs_dance(LPVOID vargp);
DWORD WINAPI thread_bPload(LPVOID vargp);
DWORD WINAPI thread_bPload_2blooms(LPVOID vargp);
#else
void *thread_process_vanity(void *vargp);
void *thread_process_minikeys(void *vargp);
void *thread_process(void *vargp);
void *thread_process_mnemonic(void *vargp);
void *thread_process_derived(void *vargp);
void *thread_process_troot(void *vargp);
void *thread_process_poetry(void *vargp);
void *thread_process_brainwallet(void *vargp);
void *thread_process_pub2addr(void *vargp);
void *thread_process_bsgs(void *vargp);
void *thread_process_bsgs_backward(void *vargp);
void *thread_process_bsgs_both(void *vargp);
void *thread_process_bsgs_random(void *vargp);
void *thread_process_bsgs_dance(void *vargp);
void *thread_bPload(void *vargp);
void *thread_bPload_2blooms(void *vargp);
#endif

char *pubkeytopubaddress(char *pkey,int length);
void pubkeytopubaddress_dst(char *pkey,int length,char *dst);
void rmd160toaddress_dst(char *rmd,char *dst);
void set_minikey(char *buffer,char *rawbuffer,int length);
bool increment_minikey_index(char *buffer,char *rawbuffer,int index);
void increment_minikey_N(char *rawbuffer);
	
void KECCAK_256(uint8_t *source, size_t size,uint8_t *dst);
void generate_binaddress_eth(Point &publickey,unsigned char *dst_address);

int THREADOUTPUT = 0;
char *bit_range_str_min;
char *bit_range_str_max;

const char *bsgs_modes[5] = {"sequential","backward","both","random","dance"};
const char *modes[11] = {"xpoint","address","bsgs","rmd160","pub2rmd","minikeys","vanity","mnemonic","poetry","brainwallet","pubkey2addr"};
const char *cryptos[12] = {"btc","eth","all","troot","bch","btg","etc","ltc","doge","xrp","sol"};
const char *publicsearch[3] = {"uncompress","compress","both"};
const char *searchmodes[7] = {"sequential","random","chaos","gravity","spiral","reverse","auto"};
const char *default_fileName = "addresses.txt";

int FLAGSEARCHMODE = SEARCHMODE_RANDOM;
double chaos_x = 0.1;
const double chaos_r = 3.99999;
Int gravity_center;
int gravity_found_count = 0;
Int spiral_center;
double spiral_angle = 0.0;
const double spiral_step = 0.1;
int auto_phase = 0;
int auto_cycles = 0;
const int AUTO_PHASE_CYCLES[4] = {200, 300, 200, 300};

int FLAGMNEMONIC_WORDS = 0;
int FLAGMNEMONIC_LANG = 0;
int FLAGMNEMONIC_ALL_LANGS = 0;
int FLAGMNEMONIC_ETH = 0;
int FLAGDP = 1;
char mnemonic_lang_name[64] = "english";
int FLAGPOETRY_WORDS = 0;
int FLAGBRAINWALLET_WORDS = 0;

int FLAGPATH = 0;
char *path_string = NULL;
uint32_t parsed_path[16];
int parsed_path_len = 0;
int FLAGVERBOSE = 0;

const int NUM_BIP39_LANGUAGES = 10;
const char *bip39_language_names[] = {
    "english", "spanish", "french", "italian", "czech",
    "portuguese", "japanese", "korean", "chinese_simplified", "chinese_traditional"
};

char *bip39_wordlist_storage[2048];
char **bip39_wordlist = bip39_wordlist_storage;
int bip39_wordlist_size = 0;

char **bip39_all_wordlists[10];
int bip39_all_sizes[10];

bool load_bip39_wordlist(const char *lang) {
	char path[512];
	snprintf(path, sizeof(path), "tests/bip39/%s.txt", lang);
	FILE *f = fopen(path, "r");
	if(!f) {
		snprintf(path, sizeof(path), "%s.txt", lang);
		f = fopen(path, "r");
	}
	if(!f) {
		fprintf(stderr, "[E] Cannot open BIP39 wordlist: %s\n", lang);
		return false;
	}
	for(int i = 0; i < bip39_wordlist_size; i++) {
		free(bip39_wordlist[i]);
		bip39_wordlist[i] = NULL;
	}
	bip39_wordlist_size = 0;
	char line[512];
	while(fgets(line, sizeof(line), f) && bip39_wordlist_size < 2048) {
		int len = strlen(line);
		while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
			line[--len] = '\0';
		}
		if(len > 0) {
			bip39_wordlist[bip39_wordlist_size] = strdup(line);
			bip39_wordlist_size++;
		}
	}
	fclose(f);
	if(bip39_wordlist_size != 2048) {
		fprintf(stderr, "[E] BIP39 wordlist '%s' has %d words (expected 2048)\n", lang, bip39_wordlist_size);
		return false;
	}
	return true;
}

void preload_all_wordlists() {
	for(int i = 0; i < NUM_BIP39_LANGUAGES; i++) {
		bip39_all_wordlists[i] = NULL;
		bip39_all_sizes[i] = 0;
		char path[512];
		snprintf(path, sizeof(path), "tests/bip39/%s.txt", bip39_language_names[i]);
		FILE *f = fopen(path, "r");
		if(!f) {
			fprintf(stderr, "[E] Cannot open BIP39 wordlist: %s (%s)\n", bip39_language_names[i], path);
			continue;
		}
		bip39_all_wordlists[i] = (char**)calloc(2048, sizeof(char*));
		if(!bip39_all_wordlists[i]) {
			fprintf(stderr, "[E] Cannot allocate memory for %s\n", bip39_language_names[i]);
			fclose(f);
			continue;
		}
		int count = 0;
		char line[512];
		while(fgets(line, sizeof(line), f) && count < 2048) {
			int len = strlen(line);
			while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
				line[--len] = '\0';
			}
			if(len > 0) {
				bip39_all_wordlists[i][count] = strdup(line);
				count++;
			}
		}
		fclose(f);
		bip39_all_sizes[i] = count;
		if(count != 2048) {
			fprintf(stderr, "[PRELOAD] Wordlist '%s' has %d/2048 words - skipping\n", bip39_language_names[i], count);
			for(int j = 0; j < count; j++) free(bip39_all_wordlists[i][j]);
			free(bip39_all_wordlists[i]);
			bip39_all_wordlists[i] = NULL;
			bip39_all_sizes[i] = 0;
		}
	}
}

// Poetry word list (words for hex encoding)
char *poetry_words[2048];
int poetry_words_size = 0;

bool load_poetry_words(const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        fprintf(stderr, "[E] Cannot open poetry wordlist: %s\n", path);
        return false;
    }
    poetry_words_size = 0;
    char line[256];
    while(fgets(line, sizeof(line), f) && poetry_words_size < 2048) {
        int len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        if(len > 0) {
            poetry_words[poetry_words_size] = strdup(line);
            poetry_words_size++;
        }
    }
    fclose(f);
    printf("[+] Loaded poetry wordlist: %d words\n", poetry_words_size);
    return true;
}

// Brainwallet word list
char *brainwallet_words[65536];
int brainwallet_words_size = 0;

bool load_brainwallet_words(const char *path) {
    FILE *f = fopen(path, "r");
    if(!f) {
        fprintf(stderr, "[E] Cannot open brainwallet wordlist: %s\n", path);
        return false;
    }
    brainwallet_words_size = 0;
    char line[256];
    while(fgets(line, sizeof(line), f) && brainwallet_words_size < 65536) {
        int len = strlen(line);
        while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        if(len > 0) {
            brainwallet_words[brainwallet_words_size] = strdup(line);
            brainwallet_words_size++;
        }
    }
    fclose(f);
    printf("[+] Loaded brainwallet wordlist: %d words\n", brainwallet_words_size);
    return true;
}

bool parse_derivation_path(const char *path) {
    if(!path || path[0] != 'm') return false;
    parsed_path_len = 0;
    const char *p = path + 1;
    while(*p && parsed_path_len < 16) {
        if(*p == '/') { p++; continue; }
        uint32_t index = 0;
        bool hardened = false;
        if(*p == '\0' || *p == '/') return false;
        while(*p >= '0' && *p <= '9') {
            index = index * 10 + (*p - '0');
            p++;
        }
        if(*p == '\'' || *p == 'H' || *p == 'h') {
            hardened = true;
            index += 0x80000000;
            p++;
        }
        parsed_path[parsed_path_len++] = index;
    }
    return parsed_path_len > 0;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
HANDLE* tid = NULL;
HANDLE write_keys;
HANDLE write_random;
HANDLE bsgs_thread;
HANDLE *bPload_mutex = NULL;
#else
pthread_t *tid = NULL;
pthread_mutex_t write_keys;
pthread_mutex_t write_random;
pthread_mutex_t bsgs_thread;
pthread_mutex_t *bPload_mutex = NULL;
#endif

uint64_t FINISHED_THREADS_COUNTER = 0;
uint64_t FINISHED_THREADS_BP = 0;
uint64_t THREADCYCLES = 0;
uint64_t THREADCOUNTER = 0;
uint64_t FINISHED_ITEMS = 0;
uint64_t OLDFINISHED_ITEMS = -1;

uint8_t byte_encode_crypto = 0x00;		/* Bitcoin  */


int vanity_rmd_targets = 0;
int vanity_rmd_total = 0;
int *vanity_rmd_limits = NULL;
uint8_t ***vanity_rmd_limit_values_A = NULL,***vanity_rmd_limit_values_B = NULL;
int vanity_rmd_minimun_bytes_check_length = 999999;
char **vanity_address_targets = NULL;
struct bloom *vanity_bloom = NULL;

struct bloom bloom;
struct binaryfuse_wrapper bf_filter;

struct bloom troot_bloom;
struct binaryfuse_wrapper troot_bf_filter;
struct troot_value *trootTable = NULL;
uint64_t N_TROOT = 0;

uint64_t *steps = NULL;
unsigned int *ends = NULL;
uint64_t N = 0;

uint64_t N_SEQUENTIAL_MAX = 0x100000000;
uint64_t DEBUGCOUNT = 0x400;
uint64_t u64range;

Int OUTPUTSECONDS;

int FLAGSKIPCHECKSUM = 0;
int FLAGENDOMORPHISM = 0;

int FLAGBLOOMMULTIPLIER = 1;
int FLAGVANITY = 0;
int FLAGBASEMINIKEY = 0;
int FLAGBSGSMODE = 0;
int FLAGDEBUG = 0;
int FLAGQUIET = 0;
int FLAGMATRIX = 0;
int KFACTOR = 1;
int FLAGNODECHECK = 0;
int MAXLENGTHADDRESS = -1;
int NTHREADS = 1;

int FLAGSAVEREADFILE = 0;
int FLAGREADEDFILE1 = 0;

char *NODE_RPC_URL = NULL;
int FLAGREADEDFILE2 = 0;
int FLAGREADEDFILE3 = 0;
int FLAGREADEDFILE4 = 0;
int FLAGUPDATEFILE1 = 0;


int FLAGSTRIDE = 0;
int FLAGSEARCH = 2;
int FLAGBITRANGE = 0;
int FLAGRANGE = 0;
int FLAGFILE = 0;
int FLAGMODE = MODE_ADDRESS;
int FLAGCRYPTO = 0;
int FLAGRAWDATA	= 0;
int FLAGRANDOM = 0;
int FLAG_N = 0;
int FLAGPRECALCUTED_P_FILE = 0;

int bitrange;
char *str_N;
char *range_start;
char *range_end;
char *str_stride;
Int stride;

uint64_t BSGS_XVALUE_RAM = 6;
uint64_t BSGS_BUFFERXPOINTLENGTH = 32;
uint64_t BSGS_BUFFERREGISTERLENGTH = 36;

/*
BSGS Variables
*/
int *bsgs_found;
std::vector<Point> OriginalPointsBSGS;
bool *OriginalPointsBSGScompressed;

uint64_t bytes;
char checksum[32],checksum_backup[32];
char buffer_bloom_file[1024];
struct bsgs_xvalue *bPtable;
struct address_value *addressTable;

struct oldbloom oldbloom_bP;

struct bloom *bloom_bP;
struct bloom *bloom_bPx2nd; //2nd Bloom filter check
struct bloom *bloom_bPx3rd; //3rd Bloom filter check

struct checksumsha256 *bloom_bP_checksums;
struct checksumsha256 *bloom_bPx2nd_checksums;
struct checksumsha256 *bloom_bPx3rd_checksums;

#if defined(_WIN64) && !defined(__CYGWIN__)
HANDLE *bloom_bP_mutex;
HANDLE *bloom_bPx2nd_mutex;
HANDLE *bloom_bPx3rd_mutex;
#else
pthread_mutex_t *bloom_bP_mutex;
pthread_mutex_t *bloom_bPx2nd_mutex;
pthread_mutex_t *bloom_bPx3rd_mutex;
#endif




uint64_t bloom_bP_totalbytes = 0;
uint64_t bloom_bP2_totalbytes = 0;
uint64_t bloom_bP3_totalbytes = 0;
uint64_t bsgs_m = 4194304;
uint64_t bsgs_m2;
uint64_t bsgs_m3;
uint64_t bsgs_aux;
uint32_t bsgs_point_number;

const char *str_limits_prefixs[7] = {"Mkeys/s","Gkeys/s","Tkeys/s","Pkeys/s","Ekeys/s","Zkeys/s","Ykeys/s"};
const char *str_limits[7] = {"1000000","1000000000","1000000000000","1000000000000000","1000000000000000000","1000000000000000000000","1000000000000000000000000"};
Int int_limits[7];




Int BSGS_GROUP_SIZE;
Int BSGS_CURRENT;
Int BSGS_R;
Int BSGS_AUX;
Int BSGS_N;
Int BSGS_N_double;
Int BSGS_M;					//M is squareroot(N)
Int BSGS_M_double;
Int BSGS_M2;				//M2 is M/32
Int BSGS_M2_double;			//M2_double is M2 * 2
Int BSGS_M3;				//M3 is M2/32
Int BSGS_M3_double;			//M3_double is M3 * 2

Int ONE;
Int ZERO;
Int MPZAUX;

Point BSGS_P;			//Original P is actually G, but this P value change over time for calculations
Point BSGS_MP;			//MP values this is m * P
Point BSGS_MP2;			//MP2 values this is m2 * P
Point BSGS_MP3;			//MP3 values this is m3 * P

Point BSGS_MP_double;			//MP2 values this is m2 * P * 2
Point BSGS_MP2_double;			//MP2 values this is m2 * P * 2
Point BSGS_MP3_double;			//MP3 values this is m3 * P * 2


std::vector<Point> BSGS_AMP2;
std::vector<Point> BSGS_AMP3;

Point point_temp,point_temp2;	//Temp value for some process

Int n_range_start;
Int n_range_end;
Int n_range_diff;
Int n_range_aux;

Int lambda,lambda2,beta,beta2;

Secp256K1 *secp;

// ============================================================
// Collider Search Modes - CPU Implementation
// ============================================================

void init_search_mode(Int *range_start, Int *range_end) {
	Int range_mid;
	switch(FLAGSEARCHMODE) {
		case SEARCHMODE_CHAOS:
			chaos_x = 0.1;
			printf("[+] Chaos mode: logistic map r=%.5f\n", chaos_r);
			break;
		case SEARCHMODE_GRAVITY:
			gravity_center.Set(range_start);
			gravity_found_count = 0;
			printf("[+] Gravity mode: adaptive search around found keys\n");
			break;
		case SEARCHMODE_SPIRAL:
			range_mid.Set(range_end);
			range_mid.Sub(range_start);
			range_mid.ShiftR(1);
			spiral_center.Set(range_start);
			spiral_center.Add(&range_mid);
			spiral_angle = 0.0;
			printf("[+] Spiral mode: Archimedean spiral from midpoint\n");
			break;
		case SEARCHMODE_REVERSE:
			printf("[+] Reverse mode: inverted BSGS baby/giant step roles\n");
			break;
		case SEARCHMODE_AUTO:
			auto_phase = 0;
			auto_cycles = 0;
			printf("[+] Auto mode: cycling through spiral->chaos->gravity->reverse\n");
			break;
		default:
			break;
	}
}

void get_next_key_chaos(Int *result, Int *range_start, Int *range_end) {
	Int range_size, temp;
	uint64_t chaos_val;
	range_size.Set(range_end);
	range_size.Sub(range_start);

	chaos_x = chaos_r * chaos_x * (1.0 - chaos_x);

	chaos_val = (uint64_t)(chaos_x * 1e16) % 10000000000000000ULL;
	temp.SetInt64(chaos_val);
	temp.Mult(&range_size);
	temp.ShiftR(53);

	result->Set(range_start);
	result->Add(&temp);
}

void get_next_key_gravity(Int *result, Int *range_start, Int *range_end) {
	Int range_size, offset, temp;
	double pull_strength;
	int rand_val;

	range_size.Set(range_end);
	range_size.Sub(range_start);

	if(gravity_found_count > 0) {
		pull_strength = 0.7;
		rand_val = rand() % 100;
		if(rand_val < 70) {
			offset.SetInt32(rand() % 1024);
			if(rand() % 2 == 0) {
				result->Set(&gravity_center);
				result->Add(&offset);
			} else {
				result->Set(&gravity_center);
				result->Sub(&offset);
			}
		} else {
			result->Rand(range_start, range_end);
		}
	} else {
		result->Rand(range_start, range_end);
	}
}

void get_next_key_spiral(Int *result, Int *range_start, Int *range_end) {
	Int range_size, offset_x, offset_y, temp;
	double x, y, r;
	int64_t ix, iy;

	range_size.Set(range_end);
	range_size.Sub(range_start);

	r = spiral_step * spiral_angle;
	x = r * cos(spiral_angle);
	y = r * sin(spiral_angle);

	ix = (int64_t)(x * 1e12) % 1000000;
	iy = (int64_t)(y * 1e12) % 1000000;

	offset_x.SetInt64(abs(ix));
	offset_y.SetInt64(abs(iy));

	result->Set(&spiral_center);
	result->Add(&offset_x);
	result->Sub(&offset_y);

	spiral_angle += 0.1;
	if(spiral_angle > 1000.0) {
		spiral_angle = 0.0;
	}
}

void get_next_key_auto(Int *result, Int *range_start, Int *range_end) {
	auto_cycles++;
	if(auto_cycles >= AUTO_PHASE_CYCLES[auto_phase]) {
		auto_cycles = 0;
		auto_phase = (auto_phase + 1) % 4;
		if(FLAGQUIET == 0) {
			const char *phase_names[] = {"SPIRAL", "CHAOS", "GRAVITY", "REVERSE"};
			printf("\r[+] Auto: switching to %s phase                        \r", phase_names[auto_phase]);
			fflush(stdout);
		}
	}

	switch(auto_phase) {
		case 0: get_next_key_spiral(result, range_start, range_end); break;
		case 1: get_next_key_chaos(result, range_start, range_end); break;
		case 2: get_next_key_gravity(result, range_start, range_end); break;
		case 3: result->Rand(range_start, range_end); break;
	}
}

void get_next_search_key(Int *result, Int *range_start, Int *range_end) {
	switch(FLAGSEARCHMODE) {
		case SEARCHMODE_SEQUENTIAL:
			result->Set(range_start);
			break;
		case SEARCHMODE_RANDOM:
			result->Rand(range_start, range_end);
			break;
		case SEARCHMODE_CHAOS:
			get_next_key_chaos(result, range_start, range_end);
			break;
		case SEARCHMODE_GRAVITY:
			get_next_key_gravity(result, range_start, range_end);
			break;
		case SEARCHMODE_SPIRAL:
			get_next_key_spiral(result, range_start, range_end);
			break;
		case SEARCHMODE_REVERSE:
			result->Rand(range_start, range_end);
			break;
		case SEARCHMODE_AUTO:
			get_next_key_auto(result, range_start, range_end);
			break;
		default:
			result->Rand(range_start, range_end);
			break;
	}
}

void notify_key_found(Int *found_key) {
	if(FLAGSEARCHMODE == SEARCHMODE_GRAVITY) {
		gravity_center.Set(found_key);
		gravity_found_count++;
		if(FLAGQUIET == 0) {
			printf("\r[+] Gravity: key found, centering search around new key   \r");
			fflush(stdout);
		}
	}
}

// ============================================================
// Mnemonic Generation (BIP39)
// ============================================================

static void secure_random_bytes(uint8_t *buf, int len, unsigned int *seed) {
#if defined(_WIN64) && !defined(__CYGWIN__)
    for(int i = 0; i < len; i++) buf[i] = (uint8_t)(rand_r(seed) ^ (rand_r(seed) >> 8));
#else
    int r = getrandom(buf, len, 0);
    if(r < len) {
        for(int i = r; i < len; i++) buf[i] = (uint8_t)rand_r(seed);
    }
#endif
}

void generate_mnemonic(char *output, int word_count, unsigned int *seed) {
	if(bip39_wordlist_size != 2048) {
		output[0] = '\0';
		return;
	}
	int strength;
	switch(word_count) {
		case 12: strength = 128; break;
		case 15: strength = 160; break;
		case 18: strength = 192; break;
		case 21: strength = 224; break;
		case 24: strength = 256; break;
		default: strength = 128; break;
	}

	int entropy_bytes = strength / 8;
	int checksum_bits = strength / 32;
	int total_bits = strength + checksum_bits;
	int word_count_calc = total_bits / 11;

	uint8_t entropy[32];
	secure_random_bytes(entropy, entropy_bytes, seed);

	uint8_t hash[32];
	sha256(entropy, entropy_bytes, hash);

	char bits[288];
	int bit_idx = 0;
	for(int i = 0; i < entropy_bytes; i++) {
		for(int b = 7; b >= 0; b--) {
			bits[bit_idx++] = (entropy[i] >> b) & 1;
		}
	}
	for(int i = 0; i < checksum_bits; i++) {
		bits[bit_idx++] = (hash[i / 8] >> (7 - (i % 8))) & 1;
	}

	output[0] = '\0';
	for(int i = 0; i < word_count_calc; i++) {
		int idx = 0;
		for(int b = 0; b < 11; b++) {
			idx = (idx << 1) | bits[i * 11 + b];
		}
		if(idx < 0 || idx >= bip39_wordlist_size) {
			output[0] = '\0';
			return;
		}
		if(i > 0) strcat(output, " ");
		strcat(output, bip39_wordlist[idx]);
	}
}

bool validate_mnemonic(const char *mnemonic) {
	if(bip39_wordlist_size != 2048 || !mnemonic || !mnemonic[0]) return false;

	char temp[512];
	strncpy(temp, mnemonic, sizeof(temp) - 1);
	temp[sizeof(temp) - 1] = '\0';

	int word_count = 0;
	char *words[24];
	char *saveptr;
	char *tok = strtok_r(temp, " ", &saveptr);
	while(tok && word_count < 24) {
		words[word_count++] = tok;
		tok = strtok_r(NULL, " ", &saveptr);
	}

	if(word_count != 12 && word_count != 15 && word_count != 18 && word_count != 21 && word_count != 24)
		return false;

	char bits[288];
	int bit_idx = 0;
	for(int i = 0; i < word_count; i++) {
		int idx = -1;
		for(int j = 0; j < 2048; j++) {
			if(strcmp(words[i], bip39_wordlist[j]) == 0) {
				idx = j;
				break;
			}
		}
		if(idx < 0) return false;
		for(int b = 10; b >= 0; b--) {
			bits[bit_idx++] = (idx >> b) & 1;
		}
	}

	int strength = word_count * 11 - (word_count * 11) / 32;
	int entropy_bytes = strength / 8;
	int checksum_bits = strength / 32;

	uint8_t entropy[32] = {0};
	for(int i = 0; i < entropy_bytes; i++) {
		for(int b = 0; b < 8; b++) {
			entropy[i] |= (bits[i * 8 + b] << (7 - b));
		}
	}

	uint8_t hash[32];
	sha256(entropy, entropy_bytes, hash);

	for(int i = 0; i < checksum_bits; i++) {
		int data_bit = bits[entropy_bytes * 8 + i];
		int hash_bit = (hash[i / 8] >> (7 - (i % 8))) & 1;
		if(data_bit != hash_bit) return false;
	}

	return true;
}

// Decode mnemonic to hex (for checking)
void mnemonic_to_hex(const char *mnemonic, char *hex_output) {
	hex_output[0] = '\0';
	int word_count = 0;
	
	// Count words
	const char *p = mnemonic;
	while(*p) {
		if(*p == ' ') word_count++;
		p++;
	}
	word_count++;
	
	// Convert each 11-bit word index to bits
	char bits[288];
	int bit_idx = 0;
	
	char temp[512];
	strncpy(temp, mnemonic, sizeof(temp));
	
	char *saveptr;
	char *word = strtok_r(temp, " ", &saveptr);
	while(word && bit_idx < 264) {
		int idx = -1;
		for(int i = 0; i < 2048; i++) {
			if(strcmp(word, bip39_wordlist[i]) == 0) {
				idx = i;
				break;
			}
		}
		if(idx >= 0) {
			for(int b = 10; b >= 0; b--) {
				bits[bit_idx++] = (idx >> b) & 1;
			}
		}
		word = strtok_r(NULL, " ", &saveptr);
	}
	
	// Convert bits to hex (skip checksum)
	int total_bits = word_count * 11 - (word_count * 11) % 8;
	
	for(int i = 0; i < total_bits; i += 4) {
		int val = 0;
		for(int b = 0; b < 4 && (i + b) < total_bits; b++) {
			val = (val << 1) | bits[i + b];
		}
		sprintf(hex_output + strlen(hex_output), "%x", val);
	}
}

void mnemonic_to_seed(const char *mnemonic, const char *passphrase, uint8_t *seed) {
	char salt[256];
	snprintf(salt, sizeof(salt), "mnemonic%s", passphrase ? passphrase : "");
	pbkdf2_hmac_sha512(seed, 64, (uint8_t *)mnemonic, strlen(mnemonic), (uint8_t *)salt, strlen(salt), 2048);
}

void bip32_master_from_seed(const uint8_t *seed, uint8_t *master_key, uint8_t *master_chain) {
	uint8_t hmac[64];
	hmac_sha512((uint8_t *)"Bitcoin seed", 12, (uint8_t *)seed, 64, hmac);
	memcpy(master_key, hmac, 32);
	memcpy(master_chain, hmac + 32, 32);
}

int bip32_derive_child(const uint8_t *parent_key, const uint8_t *parent_chain,
                       uint32_t index, uint8_t *child_key, uint8_t *child_chain) {
	uint8_t data[37];
	uint8_t hmac[64];

	if (index >= 0x80000000) {
		data[0] = 0x00;
		memcpy(data + 1, parent_key, 32);
		data[33] = (index >> 24) & 0xFF;
		data[34] = (index >> 16) & 0xFF;
		data[35] = (index >> 8) & 0xFF;
		data[36] = index & 0xFF;
		hmac_sha512((uint8_t *)parent_chain, 32, data, 37, hmac);
	} else {
		Int keyInt;
		Point pubKey;
		uint8_t pubkey_bytes[33];
		keyInt.Set32Bytes((uint8_t *)parent_key);
		pubKey = secp->ComputePublicKey(&keyInt);
		pubkey_bytes[0] = pubKey.y.IsEven() ? 0x02 : 0x03;
		pubKey.x.Get32Bytes(pubkey_bytes + 1);
		memcpy(data, pubkey_bytes, 33);
		data[33] = (index >> 24) & 0xFF;
		data[34] = (index >> 16) & 0xFF;
		data[35] = (index >> 8) & 0xFF;
		data[36] = index & 0xFF;
		hmac_sha512((uint8_t *)parent_chain, 32, data, 37, hmac);
	}

	uint8_t il[32];
	memcpy(il, hmac, 32);
	memcpy(child_chain, hmac + 32, 32);

	Int childInt, ilInt;
	childInt.Set32Bytes((uint8_t *)parent_key);
	ilInt.Set32Bytes(il);
	childInt.ModAddK1order(&childInt, &ilInt);
	if (childInt.IsZero()) return -1;
	childInt.Get32Bytes(child_key);
	return 0;
}

void bip32_derive_path(const uint8_t *master_key, const uint8_t *master_chain,
                       const uint32_t *path, int path_len,
                       uint8_t *derived_key, uint8_t *derived_chain) {
	uint8_t current_key[32], current_chain[32];
	uint8_t next_key[32], next_chain[32];
	memcpy(current_key, master_key, 32);
	memcpy(current_chain, master_chain, 32);
	for (int i = 0; i < path_len; i++) {
		if (bip32_derive_child(current_key, current_chain, path[i], next_key, next_chain) != 0) {
			memcpy(derived_key, current_key, 32);
			memcpy(derived_chain, current_chain, 32);
			return;
		}
		memcpy(current_key, next_key, 32);
		memcpy(current_chain, next_chain, 32);
	}
	memcpy(derived_key, current_key, 32);
	memcpy(derived_chain, current_chain, 32);
}

void compute_address_hash(const uint8_t *privkey_bytes, int addr_type, uint8_t *addr_hash) {
	Int keyInt;
	Point pubKey;
	keyInt.Set32Bytes((uint8_t *)privkey_bytes);
	pubKey = secp->ComputePublicKey(&keyInt);

	if (addr_type == 0) {
		secp->GetHash160(P2PKH, true, pubKey, addr_hash);
	} else if (addr_type == 1) {
		uint8_t witness[22];
		uint8_t script_hash[32];
		witness[0] = 0x00;
		witness[1] = 0x14;
		secp->GetHash160(P2PKH, true, pubKey, witness + 2);
		sha256(witness, 22, script_hash);
		ripemd160(script_hash, 32, addr_hash);
	} else {
		secp->GetHash160(BECH32, true, pubKey, addr_hash);
	}
}

// ============================================================
// Poetry Generation (Custom Hex Encoding)
// ============================================================

// Convert hex to poetry words (3 words per chunk)
void hex_to_poetry(const char *hex, char *output) {
	output[0] = '\0';
	int hex_len = strlen(hex);
	
	// Calculate how many hex chars 3 words can represent
	int bits_per_group = (int)(3.0 * log2((double)poetry_words_size));
	int hex_per_group = bits_per_group / 4;
	if(hex_per_group < 1) hex_per_group = 1;
	if(hex_per_group > 8) hex_per_group = 8;
	if(hex_per_group > hex_len) hex_per_group = hex_len;
	// Round to even
	if(hex_per_group % 2 != 0) hex_per_group--;
	if(hex_per_group < 2) hex_per_group = 2;
	
	uint64_t max_val = 1;
	for(int i = 0; i < hex_per_group * 4; i++) max_val *= 2;
	
	for(int i = 0; i < hex_len; i += hex_per_group) {
		char chunk[9] = {0};
		strncpy(chunk, hex + i, hex_per_group);
		
		uint64_t x = strtoull(chunk, NULL, 16);
		int w1 = x % poetry_words_size;
		int w2 = (x / poetry_words_size) % poetry_words_size;
		int w3 = (x / ((uint64_t)poetry_words_size * poetry_words_size)) % poetry_words_size;
		
		if(i > 0) strcat(output, " ");
		strcat(output, poetry_words[w1]);
		strcat(output, " ");
		strcat(output, poetry_words[w2]);
		strcat(output, " ");
		strcat(output, poetry_words[w3]);
	}
}

// Convert poetry words to hex (decode)
void poetry_to_hex(const char *poetry, char *hex_output) {
	hex_output[0] = '\0';
	
	// Calculate how many hex chars 3 words represent
	int bits_per_group = (int)(3.0 * log2((double)poetry_words_size));
	int hex_per_group = bits_per_group / 4;
	if(hex_per_group < 1) hex_per_group = 1;
	if(hex_per_group > 8) hex_per_group = 8;
	if(hex_per_group % 2 != 0) hex_per_group--;
	if(hex_per_group < 2) hex_per_group = 2;
	
	char temp[2048];
	strncpy(temp, poetry, sizeof(temp));
	
	char *saveptr;
	char *word = strtok_r(temp, " ", &saveptr);
	int count = 0;
	
	while(word && count < 300) {
		int w1_idx = -1, w2_idx = -1, w3_idx = -1;
		
		for(int i = 0; i < poetry_words_size; i++) {
			if(strcmp(word, poetry_words[i]) == 0) { w1_idx = i; break; }
		}
		
		word = strtok_r(NULL, " ", &saveptr);
		if(!word) break;
		for(int i = 0; i < poetry_words_size; i++) {
			if(strcmp(word, poetry_words[i]) == 0) { w2_idx = i; break; }
		}
		
		word = strtok_r(NULL, " ", &saveptr);
		if(!word) break;
		for(int i = 0; i < poetry_words_size; i++) {
			if(strcmp(word, poetry_words[i]) == 0) { w3_idx = i; break; }
		}
		
		if(w1_idx >= 0 && w2_idx >= 0 && w3_idx >= 0) {
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

// Generate random poetry
void generate_poetry(char *output, int word_count, unsigned int *seed) {
	output[0] = '\0';
	for(int i = 0; i < word_count; i++) {
		if(i > 0) strcat(output, " ");
		strcat(output, poetry_words[rand_r(seed) % poetry_words_size]);
	}
}

int main(int argc, char **argv)	{
	char buffer[2048];
	char rawvalue[32];
	struct tothread *tt;	//tothread
	Tokenizer t,tokenizerbsgs;	//tokenizer
	char *fileName = NULL;
	char *hextemp = NULL;
	char *aux = NULL;
	char *aux2 = NULL;
	char *pointx_str = NULL;
	char *pointy_str = NULL;
	char *str_seconds = NULL;
	char *str_total = NULL;
	char *str_pretotal = NULL;
	char *str_divpretotal = NULL;
	char *bf_ptr = NULL;
	char *bPload_threads_available;
	FILE *fd,*fd_aux1,*fd_aux2,*fd_aux3;
	uint64_t i,BASE,PERTHREAD_R,itemsbloom,itemsbloom2,itemsbloom3;
	uint32_t finished;
	int readed,continue_flag,check_flag,c,salir,index_value,j;
	Int total,pretotal,debugcount_mpz,seconds,div_pretotal,int_aux,int_r,int_q,int58;
	struct bPload *bPload_temp_ptr;
	size_t rsize;
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	DWORD s;
	write_keys = CreateMutex(NULL, FALSE, NULL);
	write_random = CreateMutex(NULL, FALSE, NULL);
	bsgs_thread = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&write_keys,NULL);
	pthread_mutex_init(&write_random,NULL);
	pthread_mutex_init(&bsgs_thread,NULL);
	int s;
#endif

	srand(time(NULL));

	secp = new Secp256K1();
	secp->Init();
	OUTPUTSECONDS.SetInt32(30);
	ZERO.SetInt32(0);
	ONE.SetInt32(1);
	BSGS_GROUP_SIZE.SetInt32(CPU_GRP_SIZE);
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	//Any windows secure random source goes here
	rseed(clock() + time(NULL) + rand());
#else
	unsigned long rseedvalue;
	int bytes_read = getrandom(&rseedvalue, sizeof(unsigned long), GRND_NONBLOCK);
	if(bytes_read > 0)	{
		rseed(rseedvalue);
		/*
		In any case that seed is for a failsafe RNG, the default source on linux is getrandom function
		See https://www.2uo.de/myths-about-urandom/
		*/
	}
	else	{
		/*
			what year is??
			WTF linux without RNG ? 
		*/
		fprintf(stderr,"[E] Error getrandom() ?\n");
		exit(EXIT_FAILURE);
		rseed(clock() + time(NULL) + rand()*rand());
	}
#endif
	
	
	
	printf("[+] Version %s, developed & modified by TrueScent\n",version);

	while ((c = getopt(argc, argv, "deh6MqRSVZ:B:b:c:C:D:E:f:I:k:l:m:N::n:p:r:s:t:T:v:G:8:z:x:w:L:W")) != -1) {
		switch(c) {
			case 'h':
				menu();
			break;
			case '6':
				FLAGSKIPCHECKSUM = 1;
				fprintf(stderr,"[W] Skipping checksums on files\n");
			break;
			case 'B':
				index_value = indexOf(optarg,bsgs_modes,5);
				if(index_value >= 0 && index_value <= 4)	{
					FLAGBSGSMODE = index_value;
					//printf("[+] BSGS mode %s\n",optarg);
				}
				else	{
					fprintf(stderr,"[W] Ignoring unknow bsgs mode %s\n",optarg);
				}
			break;
			case 'b':
				bitrange = strtol(optarg,NULL,10);
				if(bitrange > 0 && bitrange <=256 )	{
					MPZAUX.Set(&ONE);
					MPZAUX.ShiftL(bitrange-1);
					bit_range_str_min = MPZAUX.GetBase16();
					checkpointer((void *)bit_range_str_min,__FILE__,"malloc","bit_range_str_min" ,__LINE__ -1);
					MPZAUX.Set(&ONE);
					MPZAUX.ShiftL(bitrange);
					if(MPZAUX.IsGreater(&secp->order))	{
						MPZAUX.Set(&secp->order);
					}
					bit_range_str_max = MPZAUX.GetBase16();
					checkpointer((void *)bit_range_str_max,__FILE__,"malloc","bit_range_str_min" ,__LINE__ -1);
					FLAGBITRANGE = 1;
				}
				else	{
					fprintf(stderr,"[E] invalid bits param: %s.\n",optarg);
				}
			break;
			case 'c':
				index_value = indexOf(optarg,cryptos,11);
				switch(index_value) {
					case 0: //btc
						FLAGCRYPTO = CRYPTO_BTC;
					break;
					case 1: //eth
						FLAGCRYPTO = CRYPTO_ETH;
						printf("[+] Setting search for ETH adddress.\n");
					break;
					case 2: //all
						FLAGCRYPTO = CRYPTO_ALL;
						printf("[+] Setting search for all currencies simultaneously.\n");
					break;
					case 3: //troot
						FLAGCRYPTO = CRYPTO_TROOT;
						printf("[+] Setting search for Taproot (P2TR) addresses.\n");
					break;
					case 4: //bch
						FLAGCRYPTO = CRYPTO_BCH;
						printf("[+] Setting search for Bitcoin Cash (BCH) addresses.\n");
					break;
					case 5: //btg
						FLAGCRYPTO = CRYPTO_BTG;
						printf("[+] Setting search for Bitcoin Gold (BTG) addresses.\n");
					break;
					case 6: //etc
						FLAGCRYPTO = CRYPTO_ETC;
						printf("[+] Setting search for Ethereum Classic (ETC) addresses.\n");
					break;
					case 7: //ltc
						FLAGCRYPTO = CRYPTO_LTC;
						printf("[+] Setting search for Litecoin (LTC) addresses.\n");
					break;
					case 8: //doge
						FLAGCRYPTO = CRYPTO_DOGE;
						printf("[+] Setting search for Dogecoin (DOGE) addresses.\n");
					break;
					case 9: //xrp
						FLAGCRYPTO = CRYPTO_XRP;
						printf("[+] Setting search for XRP (Ripple) addresses.\n");
					break;
					case 10: //sol
						FLAGCRYPTO = CRYPTO_SOL;
						printf("[+] Setting search for Solana addresses.\n");
					break;
					default:
						FLAGCRYPTO = CRYPTO_NONE;
						fprintf(stderr,"[E] Unknow crypto value %s\n",optarg);
						exit(EXIT_FAILURE);
					break;
				}
			break;
			case 'C':
				if(strlen(optarg) == 22)	{
					FLAGBASEMINIKEY = 1;
					str_baseminikey = (char*) malloc(23);
					checkpointer((void *)str_baseminikey,__FILE__,"malloc","str_baseminikey" ,__LINE__ - 1);
					raw_baseminikey = (char*) malloc(23);
					checkpointer((void *)raw_baseminikey,__FILE__,"malloc","raw_baseminikey" ,__LINE__ - 1);
					strncpy(str_baseminikey,optarg,22);
					for(i = 0; i< 21; i++)	{
						if(strchr(Ccoinbuffer,str_baseminikey[i+1]) != NULL)	{
							raw_baseminikey[i] = (int)(strchr(Ccoinbuffer,str_baseminikey[i+1]) - Ccoinbuffer) % 58;
						}
						else	{
							fprintf(stderr,"[E] invalid character in minikey\n");
							exit(EXIT_FAILURE);
						}
						
					}
				}
				else	{
					fprintf(stderr,"[E] Invalid Minikey length %li : %s\n",strlen(optarg),optarg);
					exit(EXIT_FAILURE);
				}
				
			break;
			case 'd':
				FLAGDEBUG = 1;
				printf("[+] Flag DEBUG enabled\n");
			break;
			case 'e':
				FLAGENDOMORPHISM = 1;
				printf("[+] Endomorphism enabled\n");
				lambda.SetBase16("5363ad4cc05c30e0a5261c028812645a122e22ea20816678df02967c1b23bd72");
				lambda2.SetBase16("ac9c52b33fa3cf1f5ad9e3fd77ed9ba4a880b9fc8ec739c2e0cfc810b51283ce");
				beta.SetBase16("7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee");
				beta2.SetBase16("851695d49a83f8ef919bb86153cbcb16630fb68aed0a766a3ec693d68e6afa40");
			break;
			case 'f':
				FLAGFILE = 1;
				fileName = optarg;
			break;
			case 'I':
				FLAGSTRIDE = 1;
				str_stride = optarg;
			break;
			case 'k':
				KFACTOR = (int)strtol(optarg,NULL,10);
				if(KFACTOR <= 0)	{
					KFACTOR = 1;
				}
				printf("[+] K factor %i\n",KFACTOR);
			break;

			case 'l':
				switch(indexOf(optarg,publicsearch,3)) {
					case SEARCH_UNCOMPRESS:
						FLAGSEARCH = SEARCH_UNCOMPRESS;
						printf("[+] Search uncompress only\n");
					break;
					case SEARCH_COMPRESS:
						FLAGSEARCH = SEARCH_COMPRESS;
						printf("[+] Search compress only\n");
					break;
					case SEARCH_BOTH:
						FLAGSEARCH = SEARCH_BOTH;
						printf("[+] Search both compress and uncompress\n");
					break;
				}
			break;
			case 'M':
				FLAGMATRIX = 1;
				printf("[+] Matrix screen\n");
			break;
			case 'm':
				switch(indexOf(optarg,modes,11)) {
					case MODE_XPOINT: //xpoint
						FLAGMODE = MODE_XPOINT;
						printf("[+] Mode xpoint\n");
					break;
					case MODE_ADDRESS: //address
						FLAGMODE = MODE_ADDRESS;
						printf("[+] Mode address\n");
					break;
					case MODE_BSGS:
						FLAGMODE = MODE_BSGS;
						//printf("[+] Mode BSGS\n");
					break;
					case MODE_RMD160:
						FLAGMODE = MODE_RMD160;
						FLAGCRYPTO = CRYPTO_BTC;
						printf("[+] Mode rmd160\n");
					break;
					case MODE_PUB2RMD:
						FLAGMODE = MODE_PUB2RMD;
						printf("[+] Mode pub2rmd was removed\n");
						exit(0);
					break;
					case MODE_MINIKEYS:
						FLAGMODE = MODE_MINIKEYS;
						printf("[+] Mode minikeys\n");
					break;
					case MODE_VANITY:
						FLAGMODE = MODE_VANITY;
						printf("[+] Mode vanity\n");
						if(vanity_bloom == NULL){
							vanity_bloom = (struct bloom*) calloc(1,sizeof(struct bloom));
							checkpointer((void *)vanity_bloom,__FILE__,"calloc","vanity_bloom" ,__LINE__ -1);
						}
					break;
					case MODE_MNEMONIC:
						FLAGMODE = MODE_MNEMONIC;
						printf("[+] Mode mnemonic (BIP39)\n");
					break;
					case MODE_POETRY:
						FLAGMODE = MODE_POETRY;
						printf("[+] Mode poetry (hex encoding)\n");
					break;
					case MODE_BRAINWALLET:
						FLAGMODE = MODE_BRAINWALLET;
						printf("[+] Mode brainwallet\n");
					break;
					case MODE_PUB2ADDR:
						FLAGMODE = MODE_PUB2ADDR;
						FLAGSEARCHMODE = SEARCHMODE_RANDOM;
						printf("[+] Mode pubkey2addr (random pubkey->address search)\n");
						printf("[+] Defaulting to -x random\n");
					break;
					default:
						fprintf(stderr,"[E] Unknow mode value %s\n",optarg);
						exit(EXIT_FAILURE);
					break;
				}
			break;
			case 'n':
				FLAG_N = 1;
				str_N = optarg;
			break;
			case 'q':
				FLAGQUIET	= 1;
				printf("[+] Quiet thread output\n");
			break;
			case 'R':
				printf("[+] Random mode\n");
				FLAGRANDOM = 1;
				FLAGBSGSMODE =  3;
			break;
			case 'r':
				if(optarg != NULL)	{
					stringtokenizer(optarg,&t);
					switch(t.n)	{
						case 1:
							range_start = nextToken(&t);
							if(isValidHex(range_start)) {
								FLAGRANGE = 1;
								range_end = secp->order.GetBase16();
							}
							else	{
								fprintf(stderr,"[E] Invalid hexstring : %s.\n",range_start);
							}
						break;
						case 2:
							range_start = nextToken(&t);
							range_end	 = nextToken(&t);
							if(isValidHex(range_start) && isValidHex(range_end)) {
									FLAGRANGE = 1;
							}
							else	{
								if(isValidHex(range_start)) {
									fprintf(stderr,"[E] Invalid hexstring : %s\n",range_start);
								}
								else	{
									fprintf(stderr,"[E] Invalid hexstring : %s\n",range_end);
								}
							}
						break;
						default:
							printf("[E] Unknow number of Range Params: %i\n",t.n);
						break;
					}
				}
			break;
			case 's':
				OUTPUTSECONDS.SetBase10(optarg);
				if(OUTPUTSECONDS.IsLower(&ZERO))	{
					OUTPUTSECONDS.SetInt32(30);
				}
				if(OUTPUTSECONDS.IsZero())	{
					printf("[+] Turn off stats output\n");
				}
				else	{
					hextemp = OUTPUTSECONDS.GetBase10();
					printf("[+] Stats output every %s seconds\n",hextemp);
					free(hextemp);
				}
			break;
			case 'S':
				FLAGSAVEREADFILE = 1;
			break;
			case 't':
				NTHREADS = strtol(optarg,NULL,10);
				if(NTHREADS <= 0)	{
					NTHREADS = 1;
				}
				printf((NTHREADS > 1) ? "[+] Threads : %u\n": "[+] Thread : %u\n",NTHREADS);
			break;
			case 'T': {
				long timestamp = strtol(optarg, NULL, 10);
				if(timestamp <= 0) {
					fprintf(stderr, "[E] Invalid timestamp: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				FLAGRANGE = 1;
				Int ts_start, ts_end, ts_window;
				ts_start.SetInt64((uint64_t)timestamp);
				ts_window.SetInt64(0x100000000ULL);
				ts_end.Set(&ts_start);
				ts_end.Add(&ts_window);
				range_start = ts_start.GetBase16();
				range_end = ts_end.GetBase16();
				printf("[+] Timestamp: %ld (0x%s)\n", timestamp, range_start);
				printf("[+] Range: 0x%s to 0x%s\n", range_start, range_end);
				printf("[+] Searching ~4B keys around timestamp\n");
			}
			break;
			case 'Z': {
				int strip_bytes = strtol(optarg, NULL, 10);
				if(strip_bytes < 1 || strip_bytes > 31) {
					fprintf(stderr, "[E] Strip bytes must be 1-31\n");
					exit(EXIT_FAILURE);
				}
				if(!FLAGBITRANGE) {
					fprintf(stderr, "[E] -Z requires -b (bit range) to be set first\n");
					exit(EXIT_FAILURE);
				}
				FLAGRANGE = 1;
				FLAGBITRANGE = 0;
				Int range_low, range_high;
				int remaining_bits = bitrange - (strip_bytes * 8);
				if(remaining_bits < 1) {
					fprintf(stderr, "[E] Strip bytes exceeds bit range\n");
					exit(EXIT_FAILURE);
				}
				range_low.SetInt32(1);
				range_low.ShiftL(remaining_bits - 1);
				range_high.SetInt32(1);
				range_high.ShiftL(remaining_bits);
				if(range_high.IsGreater(&secp->order)) {
					range_high.Set(&secp->order);
				}
				range_start = range_low.GetBase16();
				range_end = range_high.GetBase16();
				uint64_t range_size = range_high.GetInt64() - range_low.GetInt64();
				if(FLAGMODE != MODE_BSGS && N_SEQUENTIAL_MAX > range_size) {
					uint64_t rounded = (range_size / 1024) * 1024;
					if(rounded < 1024) rounded = 1024;
					N_SEQUENTIAL_MAX = rounded;
					printf("[+] Auto-set N to %" PRIu64 " to fit range\n", N_SEQUENTIAL_MAX);
				}
				printf("[+] Stripped %d leading zero bytes\n", strip_bytes);
				printf("[+] Effective range: 0x%s to 0x%s\n", range_start, range_end);
				printf("[+] Searching %d-bit keys with %d zero bytes stripped\n", bitrange, strip_bytes);
			}
			break;
			case 'v':
				FLAGVANITY = 1;
				if(vanity_bloom == NULL){
					vanity_bloom = (struct bloom*) calloc(1,sizeof(struct bloom));
					checkpointer((void *)vanity_bloom,__FILE__,"calloc","vanity_bloom" ,__LINE__ -1);
				}
				if(isValidBase58String(optarg))	{
					if(addvanity(optarg) > 0)	{
						printf("[+] Added Vanity search : %s\n",optarg);
					}
					else	{
						printf("[+] Vanity search \"%s\" was NOT Added\n",optarg);
					}
				}
				else {
					fprintf(stderr,"[+] The string \"%s\" is not Valid Base58\n",optarg);
				}
				
			break;
			case '8':
				if(strlen(optarg) == 58)	{
					Ccoinbuffer = optarg; 
					printf("[+] Base58 for Minikeys %s\n",Ccoinbuffer);
				}
				else	{
					fprintf(stderr,"[E] The base58 alphabet must be 58 characters long.\n");
					exit(EXIT_FAILURE);
				}
			break;
			case 'z':
				FLAGBLOOMMULTIPLIER= strtol(optarg,NULL,10);
				if(FLAGBLOOMMULTIPLIER <= 0)	{
					FLAGBLOOMMULTIPLIER = 1;
				}
				printf("[+] Bloom Size Multiplier %i\n",FLAGBLOOMMULTIPLIER);
			break;
			case 'x':
				index_value = indexOf(optarg,searchmodes,7);
				if(index_value >= 0 && index_value <= 6)	{
					FLAGSEARCHMODE = index_value;
					printf("[+] Search mode: %s\n",optarg);
					if(FLAGSEARCHMODE == SEARCHMODE_RANDOM)	{
						FLAGRANDOM = 1;
						FLAGBSGSMODE = 3;
					}
				}
				else	{
					fprintf(stderr,"[W] Ignoring unknown search mode %s\n",optarg);
				}
			break;
			case 'w':
				if(FLAGMODE == MODE_BRAINWALLET) {
					FLAGBRAINWALLET_WORDS = strtol(optarg,NULL,10);
					if(FLAGBRAINWALLET_WORDS == 0) {
						printf("[+] Brainwallet words: random\n");
					} else {
						printf("[+] Brainwallet words: %i\n", FLAGBRAINWALLET_WORDS);
					}
				} else {
					FLAGMNEMONIC_WORDS = strtol(optarg,NULL,10);
					if(FLAGMNEMONIC_WORDS == 0) {
						printf("[+] Mnemonic words: random (12,15,18,21,24)\n");
					}
					else if(FLAGMNEMONIC_WORDS == 12 || FLAGMNEMONIC_WORDS == 15 || 
					        FLAGMNEMONIC_WORDS == 18 || FLAGMNEMONIC_WORDS == 21 || 
					        FLAGMNEMONIC_WORDS == 24) {
						printf("[+] Mnemonic words: %i\n", FLAGMNEMONIC_WORDS);
					}
					else {
						fprintf(stderr,"[W] Invalid word count %s, using random\n", optarg);
						FLAGMNEMONIC_WORDS = 0;
					}
				}
			break;
			case 'L':
				if(strcmp(optarg, "all") == 0) {
					FLAGMNEMONIC_ALL_LANGS = 1;
					printf("[+] Mnemonic language: all (%d languages)\n", NUM_BIP39_LANGUAGES);
				}
				else {
					index_value = indexOf(optarg, bip39_language_names, NUM_BIP39_LANGUAGES);
					if(index_value >= 0) {
						FLAGMNEMONIC_LANG = index_value;
						strncpy(mnemonic_lang_name, optarg, sizeof(mnemonic_lang_name)-1);
						printf("[+] Mnemonic language: %s\n", optarg);
					}
					else {
						fprintf(stderr,"[W] Unknown language %s, using english\n", optarg);
						FLAGMNEMONIC_LANG = 0;
						strncpy(mnemonic_lang_name, "english", sizeof(mnemonic_lang_name)-1);
					}
				}
			break;
			case 'D':
				FLAGDP = (int)strtol(optarg, NULL, 10);
				if(FLAGDP < 1) FLAGDP = 1;
				if(FLAGDP > 100) FLAGDP = 100;
				printf("[+] Derivation paths per mnemonic: %d\n", FLAGDP);
			break;
			case 'W':
				FLAGMNEMONIC_ETH = 1;
				printf("[+] Mnemonic mode: Ethereum (keccak256)\n");
			break;
			case 'V':
				FLAGVERBOSE = 1;
				printf("[+] Verbose derivation path output\n");
			break;
			case 'N':
				FLAGNODECHECK = 1;
				if(optarg) {
					NODE_RPC_URL = optarg;
					printf("[+] Node balance checking enabled via: %s\n", NODE_RPC_URL);
				} else {
					printf("[+] Node balance checking enabled (using public APIs)\n");
				}
			break;
			case 'p':
				FLAGPATH = 1;
				path_string = optarg;
				if(!parse_derivation_path(path_string)) {
					fprintf(stderr,"[E] Invalid derivation path: %s\n", path_string);
					exit(EXIT_FAILURE);
				}
				printf("[+] Custom derivation path: %s (%d components)\n", path_string, parsed_path_len);
			break;
			default:
				fprintf(stderr,"[E] Unknow opcion -%c\n",c);
				exit(EXIT_FAILURE);
			break;
		}
	}
	
	if(  FLAGBSGSMODE == MODE_BSGS && FLAGENDOMORPHISM)	{
		fprintf(stderr,"[E] Endomorphism doesn't work with BSGS\n");
		exit(EXIT_FAILURE);
	}
	if(FLAGPATH && FLAGMODE != MODE_ADDRESS && FLAGMODE != MODE_RMD160) {
		fprintf(stderr,"[E] -p (derivation path) only works with -m address or -m rmd160\n");
		exit(EXIT_FAILURE);
	}
	
	
	if(  FLAGBSGSMODE == MODE_BSGS  && FLAGSTRIDE)	{
		fprintf(stderr,"[E] Stride doesn't work with BSGS\n");
		exit(EXIT_FAILURE);
	}
	if(FLAGSTRIDE)	{
		if(str_stride[0] == '0' && str_stride[1] == 'x')	{
			stride.SetBase16(str_stride+2);
		}
		else{
			stride.SetBase10(str_stride);
		}
		printf("[+] Stride : %s\n",stride.GetBase10());
	}
	else	{
		FLAGSTRIDE = 1;
		stride.Set(&ONE);
	}
	init_generator();
	if(FLAGMODE == MODE_BSGS )	{
		printf("[+] Mode BSGS %s\n",bsgs_modes[FLAGBSGSMODE]);
	}
	
	if(FLAGFILE == 0) {
		fileName =(char*) default_fileName;
	}
	
	if((FLAGMODE == MODE_ADDRESS || FLAGMODE == MODE_PUB2ADDR) && FLAGCRYPTO == CRYPTO_NONE) {	//When none crypto is defined the default search is for Bitcoin
		FLAGCRYPTO = CRYPTO_BTC;
		printf("[+] Setting search for btc adddress\n");
	}
	if(FLAGRANGE) {
		n_range_start.SetBase16(range_start);
		if(n_range_start.IsZero())	{
			n_range_start.AddOne();
		}
		n_range_end.SetBase16(range_end);
		if(n_range_start.IsEqual(&n_range_end) == false ) {
			if(  n_range_start.IsLower(&secp->order) &&  n_range_end.IsLowerOrEqual(&secp->order) )	{
				if( n_range_start.IsGreater(&n_range_end)) {
					fprintf(stderr,"[W] Opps, start range can't be great than end range. Swapping them\n");
					n_range_aux.Set(&n_range_start);
					n_range_start.Set(&n_range_end);
					n_range_end.Set(&n_range_aux);
				}
				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
			}
			else	{
				fprintf(stderr,"[E] Start and End range can't be great than N\nFallback to random mode!\n");
				FLAGRANGE = 0;
			}
		}
		else	{
			fprintf(stderr,"[E] Start and End range can't be the same\nFallback to random mode!\n");
			FLAGRANGE = 0;
		}
	}
	if(FLAGMODE != MODE_BSGS && FLAGMODE != MODE_MINIKEYS)	{
		BSGS_N.SetInt32(DEBUGCOUNT);
		if(FLAGRANGE == 0 && FLAGBITRANGE == 0)	{
			n_range_start.SetInt32(1);
			n_range_end.Set(&secp->order);
			n_range_diff.Set(&n_range_end);
			n_range_diff.Sub(&n_range_start);
		}
		else	{
			if(FLAGBITRANGE)	{
				n_range_start.SetBase16(bit_range_str_min);
				n_range_end.SetBase16(bit_range_str_max);
				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
			}
			else	{
				if(FLAGRANGE == 0)	{
					fprintf(stderr,"[W] WTF!\n");
				}
			}
		}
	}
	N = 0;
	
	if(FLAGMODE != MODE_BSGS )	{
		if(FLAG_N){
			if(str_N[0] == '0' && str_N[1] == 'x')	{
				N_SEQUENTIAL_MAX =strtol(str_N,NULL,16);
			}
			else	{
				N_SEQUENTIAL_MAX =strtol(str_N,NULL,10);
			}
			
			if(N_SEQUENTIAL_MAX < 1024)	{
				fprintf(stderr,"[I] n value need to be equal or great than 1024, back to defaults\n");
				FLAG_N = 0;
				N_SEQUENTIAL_MAX = 0x100000000;
			}
			if(N_SEQUENTIAL_MAX % 1024 != 0)	{
				fprintf(stderr,"[I] n value need to be multiplier of  1024\n");
				FLAG_N = 0;
				N_SEQUENTIAL_MAX = 0x100000000;
			}
		}
		printf("[+] N = %p\n",(void*)N_SEQUENTIAL_MAX);
		if(FLAGMODE == MODE_MINIKEYS)	{
			BSGS_N.SetInt32(DEBUGCOUNT);
			if(FLAGBASEMINIKEY)	{
				printf("[+] Base Minikey : %s\n",str_baseminikey);
			}
			minikeyN = (char*) malloc(22);
			checkpointer((void *)minikeyN,__FILE__,"malloc","minikeyN" ,__LINE__ -1);
			i =0;
			int58.SetInt32(58);
			int_aux.SetInt64(N_SEQUENTIAL_MAX);
			int_aux.Mult(253);	
			/* We get approximately one valid mini key for each 256 candidates mini keys since this is only statistics we multiply N_SEQUENTIAL_MAX by 253 to ensure not missed one one candidate minikey between threads... in this approach we repeat from 1 to 3 candidates in each N_SEQUENTIAL_MAX cycle IF YOU FOUND some other workaround please let me know */
			i = 20;
			salir = 0;
			do	{
				if(!int_aux.IsZero())	{
					int_r.Set(&int_aux);
					int_r.Mod(&int58);
					int_q.Set(&int_aux);
					minikeyN[i] = (uint8_t)int_r.GetInt64();
					int_q.Sub(&int_r);
					int_q.Div(&int58);
					int_aux.Set(&int_q);
					i--;
				}
				else	{
					salir =1;
				}
			}while(!salir && i > 0);
			minikey_n_limit = 21 -i;
		}
		else	{
			if(FLAGBITRANGE)	{	// Bit Range
				printf("[+] Bit Range %i\n",bitrange);
			}
			else	{
				printf("[+] Range \n");
			}
		}
		if(FLAGMODE != MODE_MINIKEYS)	{
			hextemp = n_range_start.GetBase16();
			printf("[+] -- from : 0x%s\n",hextemp);
			free(hextemp);
			hextemp = n_range_end.GetBase16();
			printf("[+] -- to   : 0x%s\n",hextemp);
			free(hextemp);
		}

		switch(FLAGMODE)	{
			case MODE_MINIKEYS:
			case MODE_RMD160:
			case MODE_ADDRESS:
			case MODE_XPOINT:
			case MODE_MNEMONIC:
			case MODE_POETRY:
			case MODE_BRAINWALLET:
			case MODE_PUB2ADDR:
				if(!readFileAddress(fileName))	{
					fprintf(stderr,"[E] Unenexpected error\n");
					exit(EXIT_FAILURE);
				}
			break;
			case MODE_VANITY:
				if(!readFileVanity(fileName))	{
					fprintf(stderr,"[E] Unenexpected error\n");
					exit(EXIT_FAILURE);
				}
			break;
		}
		
		if(FLAGMODE != MODE_VANITY && !FLAGREADEDFILE1)	{
			printf("[+] Sorting data ...");
			_sort(addressTable,N);
			printf(" done! %" PRIu64 " values were loaded and sorted\n",N);
			writeFileIfNeeded(fileName);
		}
	}
	
	if(FLAGMODE == MODE_BSGS )	{
		printf("[+] Opening file %s\n",fileName);
		fd = fopen(fileName,"rb");
		if(fd == NULL)	{
			fprintf(stderr,"[E] Can't open file %s\n",fileName);
			exit(EXIT_FAILURE);
		}
		aux = (char*) malloc(1024);
		checkpointer((void *)aux,__FILE__,"malloc","aux" ,__LINE__ - 1);
		while(!feof(fd))	{
			if(fgets(aux,1022,fd) == aux)	{
				trim(aux," \t\n\r");
				if(strlen(aux) >= 128)	{	//Length of a full address in hexadecimal without 04
						N++;
				}else	{
					if(strlen(aux) >= 66)	{
						N++;
					}
				}
			}
		}
		if(N == 0)	{
			fprintf(stderr,"[E] There is no valid data in the file\n");
			exit(EXIT_FAILURE);
		}
		bsgs_found = (int*) calloc(N,sizeof(int));
		checkpointer((void *)bsgs_found,__FILE__,"calloc","bsgs_found" ,__LINE__ -1 );
		OriginalPointsBSGS.resize(N);
		OriginalPointsBSGScompressed = (bool*) malloc(N*sizeof(bool));
		checkpointer((void *)OriginalPointsBSGScompressed,__FILE__,"malloc","OriginalPointsBSGScompressed" ,__LINE__ -1 );
		pointx_str = (char*) malloc(65);
		checkpointer((void *)pointx_str,__FILE__,"malloc","pointx_str" ,__LINE__ -1 );
		pointy_str = (char*) malloc(65);
		checkpointer((void *)pointy_str,__FILE__,"malloc","pointy_str" ,__LINE__ -1 );
		fseek(fd,0,SEEK_SET);
		i = 0;
		while(!feof(fd))	{
			if(fgets(aux,1022,fd) == aux)	{
				trim(aux," \t\n\r");
				if(strlen(aux) >= 66)	{
					stringtokenizer(aux,&tokenizerbsgs);
					aux2 = nextToken(&tokenizerbsgs);
					memset(pointx_str,0,65);
					memset(pointy_str,0,65);
					switch(strlen(aux2))	{
						case 66:	//Compress

							if(secp->ParsePublicKeyHex(aux2,OriginalPointsBSGS[i],OriginalPointsBSGScompressed[i]))	{
								i++;
							}
							else	{
								N--;
							}

						break;
						case 130:	//With the 04

							if(secp->ParsePublicKeyHex(aux2,OriginalPointsBSGS[i],OriginalPointsBSGScompressed[i]))	{
								i++;
							}
							else	{
								N--;
							}

						break;
						default:
							printf("Invalid length: %s\n",aux2);
							N--;
						break;
					}
					freetokenizer(&tokenizerbsgs);
				}
			}
		}
		fclose(fd);
		bsgs_point_number = N;
		if(bsgs_point_number > 0)	{
			printf("[+] Added %u points from file\n",bsgs_point_number);
		}
		else	{
			fprintf(stderr,"[E] The file don't have any valid publickeys\n");
			exit(EXIT_FAILURE);
		}
		BSGS_N.SetInt32(0);
		BSGS_M.SetInt32(0);
		

		BSGS_M.SetInt64(bsgs_m);


		if(FLAG_N)	{	//Custom N by the -n param
						
			/* Here we need to validate if the given string is a valid hexadecimal number or a base 10 number*/
			
			/* Now the conversion*/
			if(str_N[0] == '0' && str_N[1] == 'x' )	{	/*We expected a hexadecimal value after 0x  -> str_N +2 */
				BSGS_N.SetBase16((char*)(str_N+2));
			}
			else	{
				BSGS_N.SetBase10(str_N);
			}
			
		}
		else	{	//Default N
			BSGS_N.SetInt64((uint64_t)0x100000000000);
		}

		if(BSGS_N.HasSqrt())	{	//If the root is exact
			BSGS_M.Set(&BSGS_N);
			BSGS_M.ModSqrt();
		}
		else	{
			fprintf(stderr,"[E] -n param doesn't have exact square root\n");
			exit(EXIT_FAILURE);
		}

		BSGS_AUX.Set(&BSGS_M);
		BSGS_AUX.Mod(&BSGS_GROUP_SIZE);	
		
		if(!BSGS_AUX.IsZero()){ //If M is not divisible by  BSGS_GROUP_SIZE (1024) 
			hextemp = BSGS_GROUP_SIZE.GetBase10();
			fprintf(stderr,"[E] M value is not divisible by %s\n",hextemp);
			exit(EXIT_FAILURE);
		}

		bsgs_m = BSGS_M.GetInt64();

		if(FLAGRANGE || FLAGBITRANGE)	{
			if(FLAGBITRANGE)	{	// Bit Range
				n_range_start.SetBase16(bit_range_str_min);
				n_range_end.SetBase16(bit_range_str_max);

				n_range_diff.Set(&n_range_end);
				n_range_diff.Sub(&n_range_start);
				printf("[+] Bit Range %i\n",bitrange);
				printf("[+] -- from : 0x%s\n",bit_range_str_min);
				printf("[+] -- to   : 0x%s\n",bit_range_str_max);
			}
			else	{
				printf("[+] Range \n");
				printf("[+] -- from : 0x%s\n",range_start);
				printf("[+] -- to   : 0x%s\n",range_end);
			}
		}
		else	{	//Random start

			n_range_start.SetInt32(1);
			n_range_end.Set(&secp->order);
			n_range_diff.Rand(&n_range_start,&n_range_end);
			n_range_start.Set(&n_range_diff);
		}
		BSGS_CURRENT.Set(&n_range_start);


		if(n_range_diff.IsLower(&BSGS_N) )	{
			uint64_t max_n = n_range_diff.GetInt64();
			max_n = (max_n / 1024) * 1024;
			if(max_n < 1024) {
				fprintf(stderr,"[E] the given range is too small for BSGS\n");
				exit(EXIT_FAILURE);
			}
			BSGS_N.SetInt64(max_n);
			bsgs_m = (uint64_t)sqrt((double)max_n);
			bsgs_m = (bsgs_m / 1024) * 1024;
			if(bsgs_m < 1024) bsgs_m = 1024;
			BSGS_M.SetInt64(bsgs_m);
			printf("[+] Auto-adjusted BSGS_N to %" PRIu64 " to fit range\n", max_n);
			printf("[+] Auto-adjusted baby-step size to %" PRIu64 "\n", bsgs_m);
		}
		
		/*
	M	2199023255552
		109951162777.6
	M2	109951162778
		5497558138.9
	M3	5497558139
		*/

		BSGS_M.Mult((uint64_t)KFACTOR);
		BSGS_AUX.SetInt32(32);
		BSGS_R.Set(&BSGS_M);
		BSGS_R.Mod(&BSGS_AUX);
		BSGS_M2.Set(&BSGS_M);
		BSGS_M2.Div(&BSGS_AUX);

		if(!BSGS_R.IsZero())	{ /* If BSGS_M modulo 32 is not 0*/
			BSGS_M2.AddOne();
		}
		
		BSGS_M_double.SetInt32(2);
		BSGS_M_double.Mult(&BSGS_M);
		
		
		BSGS_M2_double.SetInt32(2);
		BSGS_M2_double.Mult(&BSGS_M2);
		
		BSGS_R.Set(&BSGS_M2);
		BSGS_R.Mod(&BSGS_AUX);
		
		BSGS_M3.Set(&BSGS_M2);
		BSGS_M3.Div(&BSGS_AUX);
		
		if(!BSGS_R.IsZero())	{ /* If BSGS_M2 modulo 32 is not 0*/
			BSGS_M3.AddOne();
		}
		
		BSGS_M3_double.SetInt32(2);
		BSGS_M3_double.Mult(&BSGS_M3);
		
		bsgs_m2 =  BSGS_M2.GetInt64();
		bsgs_m3 =  BSGS_M3.GetInt64();
		
		BSGS_AUX.Set(&BSGS_N);
		BSGS_AUX.Div(&BSGS_M);
		
		BSGS_R.Set(&BSGS_N);
		BSGS_R.Mod(&BSGS_M);

		if(!BSGS_R.IsZero())	{ /* if BSGS_N modulo BSGS_M is not 0*/
			BSGS_N.Set(&BSGS_M);
			BSGS_N.Mult(&BSGS_AUX);
		}

		bsgs_m = BSGS_M.GetInt64();
		bsgs_aux = BSGS_AUX.GetInt64();
		
		
		BSGS_N_double.SetInt32(2);
		BSGS_N_double.Mult(&BSGS_N);

		
		hextemp = BSGS_N.GetBase16();
		printf("[+] N = 0x%s\n",hextemp);
		free(hextemp);
		if(((uint64_t)(bsgs_m/256)) > 10000)	{
			itemsbloom = (uint64_t)(bsgs_m / 256);
			if(bsgs_m % 256 != 0 )	{
				itemsbloom++;
			}
		}
		else{
			itemsbloom = 1000;
		}
		
		if(((uint64_t)(bsgs_m2/256)) > 1000)	{
			itemsbloom2 = (uint64_t)(bsgs_m2 / 256);
			if(bsgs_m2 % 256 != 0)	{
				itemsbloom2++;
			}
		}
		else	{
			itemsbloom2 = 1000;
		}
		
		if(((uint64_t)(bsgs_m3/256)) > 1000)	{
			itemsbloom3 = (uint64_t)(bsgs_m3/256);
			if(bsgs_m3 % 256 != 0 )	{
				itemsbloom3++;
			}
		}
		else	{
			itemsbloom3 = 1000;
		}
		
		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m);
		bloom_bP = (struct bloom*)calloc(256,sizeof(struct bloom));
		checkpointer((void *)bloom_bP,__FILE__,"calloc","bloom_bP" ,__LINE__ -1 );
		bloom_bP_checksums = (struct checksumsha256*)calloc(256,sizeof(struct checksumsha256));
		checkpointer((void *)bloom_bP_checksums,__FILE__,"calloc","bloom_bP_checksums" ,__LINE__ -1 );
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bP_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
		
#else
		bloom_bP_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
		checkpointer((void *)bloom_bP_mutex,__FILE__,"calloc","bloom_bP_mutex" ,__LINE__ -1 );
		

		fflush(stdout);
		bloom_bP_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bP_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bP_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bP[i],itemsbloom,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init _ [%" PRIu64 "]\n",i);
				exit(EXIT_FAILURE);
			}
			bloom_bP_totalbytes += bloom_bP[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bP[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP_totalbytes/(float)(uint64_t)1048576));


		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m2);
		
#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bPx2nd_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
#else
		bloom_bPx2nd_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
		checkpointer((void *)bloom_bPx2nd_mutex,__FILE__,"calloc","bloom_bPx2nd_mutex" ,__LINE__ -1 );
		bloom_bPx2nd = (struct bloom*)calloc(256,sizeof(struct bloom));
		checkpointer((void *)bloom_bPx2nd,__FILE__,"calloc","bloom_bPx2nd" ,__LINE__ -1 );
		bloom_bPx2nd_checksums = (struct checksumsha256*) calloc(256,sizeof(struct checksumsha256));
		checkpointer((void *)bloom_bPx2nd_checksums,__FILE__,"calloc","bloom_bPx2nd_checksums" ,__LINE__ -1 );
		bloom_bP2_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bPx2nd_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bPx2nd_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bPx2nd[i],itemsbloom2,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init _ [%" PRIu64 "]\n",i);
				exit(EXIT_FAILURE);
			}
			bloom_bP2_totalbytes += bloom_bPx2nd[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bPx2nd[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP2_totalbytes/(float)(uint64_t)1048576));
		

#if defined(_WIN64) && !defined(__CYGWIN__)
		bloom_bPx3rd_mutex = (HANDLE*) calloc(256,sizeof(HANDLE));
#else
		bloom_bPx3rd_mutex = (pthread_mutex_t*) calloc(256,sizeof(pthread_mutex_t));
#endif
		checkpointer((void *)bloom_bPx3rd_mutex,__FILE__,"calloc","bloom_bPx3rd_mutex" ,__LINE__ -1 );
		bloom_bPx3rd = (struct bloom*)calloc(256,sizeof(struct bloom));
		checkpointer((void *)bloom_bPx3rd,__FILE__,"calloc","bloom_bPx3rd" ,__LINE__ -1 );
		bloom_bPx3rd_checksums = (struct checksumsha256*) calloc(256,sizeof(struct checksumsha256));
		checkpointer((void *)bloom_bPx3rd_checksums,__FILE__,"calloc","bloom_bPx3rd_checksums" ,__LINE__ -1 );
		
		printf("[+] Bloom filter for %" PRIu64 " elements ",bsgs_m3);
		bloom_bP3_totalbytes = 0;
		for(i=0; i< 256; i++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			bloom_bPx3rd_mutex[i] = CreateMutex(NULL, FALSE, NULL);
#else
			pthread_mutex_init(&bloom_bPx3rd_mutex[i],NULL);
#endif
			if(bloom_init2(&bloom_bPx3rd[i],itemsbloom3,0.000001)	== 1){
				fprintf(stderr,"[E] error bloom_init [%" PRIu64 "]\n",i);
				exit(EXIT_FAILURE);
			}
			bloom_bP3_totalbytes += bloom_bPx3rd[i].bytes;
			//if(FLAGDEBUG) bloom_print(&bloom_bPx3rd[i]);
		}
		printf(": %.2f MB\n",(float)((float)(uint64_t)bloom_bP3_totalbytes/(float)(uint64_t)1048576));
		//if(FLAGDEBUG) printf("[D] bloom_bP3_totalbytes : %" PRIu64 "\n",bloom_bP3_totalbytes);




		BSGS_MP = secp->ComputePublicKey(&BSGS_M);
		BSGS_MP_double = secp->ComputePublicKey(&BSGS_M_double);
		BSGS_MP2 = secp->ComputePublicKey(&BSGS_M2);
		BSGS_MP2_double = secp->ComputePublicKey(&BSGS_M2_double);
		BSGS_MP3 = secp->ComputePublicKey(&BSGS_M3);
		BSGS_MP3_double = secp->ComputePublicKey(&BSGS_M3_double);
		
		BSGS_AMP2.reserve(32);
		BSGS_AMP3.reserve(32);
		GSn.resize(CPU_GRP_SIZE/2);

		i= 0;


		/* New aMP table just to keep the same code of JLP */
		/* Auxiliar Points to speed up calculations for the main bloom filter check */
		Point bsP = secp->Negation(BSGS_MP_double);
		Point g = bsP;
		GSn[0] = g;

		g = secp->DoubleDirect(g);
		GSn[1] = g;
		
		for(int i = 2; i < CPU_GRP_SIZE / 2; i++) {
			g = secp->AddDirect(g,bsP);
			GSn[i] = g;
		}
		
		/* For next center point */
		_2GSn = secp->DoubleDirect(GSn[CPU_GRP_SIZE / 2 - 1]);
				
		i = 0;
		point_temp.Set(BSGS_MP2);
		BSGS_AMP2[0] = secp->Negation(point_temp);
		BSGS_AMP2[0].Reduce();
		point_temp.Set(BSGS_MP2_double);
		point_temp = secp->Negation(point_temp);
		point_temp.Reduce();
		
		for(i = 1; i < 32; i++)	{
			BSGS_AMP2[i] = secp->AddDirect(BSGS_AMP2[i-1],point_temp);
			BSGS_AMP2[i].Reduce();
		}
		
		i  = 0;
		point_temp.Set(BSGS_MP3);
		BSGS_AMP3[0] = secp->Negation(point_temp);
		BSGS_AMP3[0].Reduce();
		point_temp.Set(BSGS_MP3_double);
		point_temp = secp->Negation(point_temp);
		point_temp.Reduce();

		for(i = 1; i < 32; i++)	{
			BSGS_AMP3[i] = secp->AddDirect(BSGS_AMP3[i-1],point_temp);
			BSGS_AMP3[i].Reduce();
		}

		bytes = (uint64_t)bsgs_m3 * (uint64_t) sizeof(struct bsgs_xvalue);
		printf("[+] Allocating %.2f MB for %" PRIu64  " bP Points\n",(double)(bytes/1048576),bsgs_m3);
		
		bPtable = (struct bsgs_xvalue*) malloc(bytes);
		checkpointer((void *)bPtable,__FILE__,"malloc","bPtable" ,__LINE__ -1 );
		memset(bPtable,0,bytes);
		
		if(FLAGSAVEREADFILE)	{
			/*Reading file for 1st bloom filter */

			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_4_%" PRIu64 ".blm",bsgs_m);
			fd_aux1 = fopen(buffer_bloom_file,"rb");
			if(fd_aux1 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bP[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bP[i],sizeof(struct bloom),1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					bloom_bP[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					readed = fread(&bloom_bP_checksums[i],sizeof(struct checksumsha256),1,fd_aux1);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					if(FLAGSKIPCHECKSUM == 0)	{
						sha256((uint8_t*)bloom_bP[i].bf,bloom_bP[i].bytes,(uint8_t*)rawvalue);
						if(memcmp(bloom_bP_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bP_checksums[i].backup,rawvalue,32) != 0 )	{	/* Verification */
							fprintf(stderr,"[E] Error checksum file mismatch! %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
					}
					if(i % 64 == 0 )	{
						printf(".");
						fflush(stdout);
					}
				}
				printf(" Done!\n");
				fclose(fd_aux1);
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_3_%" PRIu64 ".blm",bsgs_m);
				fd_aux1 = fopen(buffer_bloom_file,"rb");
				if(fd_aux1 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux1);
				}
				FLAGREADEDFILE1 = 1;
			}
			else	{	/*Checking for old file    keyhunt_bsgs_3_   */
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_3_%" PRIu64 ".blm",bsgs_m);
				fd_aux1 = fopen(buffer_bloom_file,"rb");
				if(fd_aux1 != NULL)	{
					printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						bf_ptr = (char*) bloom_bP[i].bf;	/*We need to save the current bf pointer*/
						readed = fread(&oldbloom_bP,sizeof(struct oldbloom),1,fd_aux1);
						
						/*
						if(FLAGDEBUG)	{
							printf("old Bloom filter %i\n",i);
							oldbloom_print(&oldbloom_bP);
						}
						*/
						
						if(readed != 1)	{
							fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						memcpy(&bloom_bP[i],&oldbloom_bP,sizeof(struct bloom));//We only need to copy the part data to the new bloom size, not from the old size
						bloom_bP[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
						
						readed = fread(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						memcpy(bloom_bP_checksums[i].data,oldbloom_bP.checksum,32);
						memcpy(bloom_bP_checksums[i].backup,oldbloom_bP.checksum_backup,32);
						memset(rawvalue,0,32);
						if(FLAGSKIPCHECKSUM == 0)	{
							sha256((uint8_t*)bloom_bP[i].bf,bloom_bP[i].bytes,(uint8_t*)rawvalue);
							if(memcmp(bloom_bP_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bP_checksums[i].backup,rawvalue,32) != 0 )	{	/* Verification */
								fprintf(stderr,"[E] Error checksum file mismatch! %s\n",buffer_bloom_file);
								exit(EXIT_FAILURE);
							}
						}
						if(i % 32 == 0 )	{
							printf(".");
							fflush(stdout);
						}
					}
					printf(" Done!\n");
					fclose(fd_aux1);
					FLAGUPDATEFILE1 = 1;	/* Flag to migrate the data to the new File keyhunt_bsgs_4_ */
					FLAGREADEDFILE1 = 1;
					
				}
				else	{
					FLAGREADEDFILE1 = 0;
					//Flag to make the new file
				}
			}
			
			/*Reading file for 2nd bloom filter */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_6_%" PRIu64 ".blm",bsgs_m2);
			fd_aux2 = fopen(buffer_bloom_file,"rb");
			if(fd_aux2 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bPx2nd[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bPx2nd[i],sizeof(struct bloom),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					bloom_bPx2nd[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					readed = fread(&bloom_bPx2nd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					memset(rawvalue,0,32);
					if(FLAGSKIPCHECKSUM == 0)	{								
						sha256((uint8_t*)bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,(uint8_t*)rawvalue);
						if(memcmp(bloom_bPx2nd_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bPx2nd_checksums[i].backup,rawvalue,32) != 0 )	{		/* Verification */
							fprintf(stderr,"[E] Error checksum file mismatch! %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
					}
					if(i % 64 == 0)	{
						printf(".");
						fflush(stdout);
					}
				}
				fclose(fd_aux2);
				printf(" Done!\n");
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_5_%" PRIu64 ".blm",bsgs_m2);
				fd_aux2 = fopen(buffer_bloom_file,"rb");
				if(fd_aux2 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux2);
				}
				memset(buffer_bloom_file,0,1024);
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_1_%" PRIu64 ".blm",bsgs_m2);
				fd_aux2 = fopen(buffer_bloom_file,"rb");
				if(fd_aux2 != NULL)	{
					printf("[W] Unused file detected %s you can delete it without worry\n",buffer_bloom_file);
					fclose(fd_aux2);
				}
				FLAGREADEDFILE2 = 1;
			}
			else	{	
				FLAGREADEDFILE2 = 0;
			}
			
			/*Reading file for bPtable */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_2_%" PRIu64 ".tbl",bsgs_m3);
			fd_aux3 = fopen(buffer_bloom_file,"rb");
			if(fd_aux3 != NULL)	{
				printf("[+] Reading bP Table from file %s .",buffer_bloom_file);
				fflush(stdout);
				rsize = fread(bPtable,bytes,1,fd_aux3);
				if(rsize != 1)	{
					fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
				rsize = fread(checksum,32,1,fd_aux3);
				if(rsize != 1)	{
					fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
				if(FLAGSKIPCHECKSUM == 0)	{
					sha256((uint8_t*)bPtable,bytes,(uint8_t*)checksum_backup);
					if(memcmp(checksum,checksum_backup,32) != 0)	{
						fprintf(stderr,"[E] Error checksum file mismatch! %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
				}
				printf("... Done!\n");
				fclose(fd_aux3);
				FLAGREADEDFILE3 = 1;
			}
			else	{
				FLAGREADEDFILE3 = 0;
			}
			
			/*Reading file for 3rd bloom filter */
			snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_7_%" PRIu64 ".blm",bsgs_m3);
			fd_aux2 = fopen(buffer_bloom_file,"rb");
			if(fd_aux2 != NULL)	{
				printf("[+] Reading bloom filter from file %s ",buffer_bloom_file);
				fflush(stdout);
				for(i = 0; i < 256;i++)	{
					bf_ptr = (char*) bloom_bPx3rd[i].bf;	/*We need to save the current bf pointer*/
					readed = fread(&bloom_bPx3rd[i],sizeof(struct bloom),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					bloom_bPx3rd[i].bf = (uint8_t*)bf_ptr;	/* Restoring the bf pointer*/
					readed = fread(bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					readed = fread(&bloom_bPx3rd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error reading the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					memset(rawvalue,0,32);
					if(FLAGSKIPCHECKSUM == 0)	{							
						sha256((uint8_t*)bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,(uint8_t*)rawvalue);
						if(memcmp(bloom_bPx3rd_checksums[i].data,rawvalue,32) != 0 || memcmp(bloom_bPx3rd_checksums[i].backup,rawvalue,32) != 0 )	{		/* Verification */
							fprintf(stderr,"[E] Error checksum file mismatch! %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
					}
					if(i % 64 == 0)	{
						printf(".");
						fflush(stdout);
					}
				}
				fclose(fd_aux2);
				printf(" Done!\n");
				FLAGREADEDFILE4 = 1;
			}
			else	{
				FLAGREADEDFILE4 = 0;
			}
			
		}
		
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE3 || !FLAGREADEDFILE4)	{
			if(FLAGREADEDFILE1 == 1)	{
				/* 
					We need just to make File 2 to File 4 this is
					- Second bloom filter 5%
					- third  bloom fitler 0.25 %
					- bp Table 0.25 %
				*/
				printf("[I] We need to recalculate some files, don't worry this is only 3%% of the previous work\n");
				FINISHED_THREADS_COUNTER = 0;
				FINISHED_THREADS_BP = 0;
				FINISHED_ITEMS = 0;
				salir = 0;
				BASE = 0;
				THREADCOUNTER = 0;
				if(THREADBPWORKLOAD >= bsgs_m2)	{
					THREADBPWORKLOAD = bsgs_m2;
				}
				THREADCYCLES = bsgs_m2 / THREADBPWORKLOAD;
				PERTHREAD_R = bsgs_m2 % THREADBPWORKLOAD;
				if(PERTHREAD_R != 0)	{
					THREADCYCLES++;
				}
				
				printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
				fflush(stdout);
				
#if defined(_WIN64) && !defined(__CYGWIN__)
				tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
				checkpointer((void *)tid,__FILE__,"calloc","tid" ,__LINE__ -1 );
				bPload_mutex = (HANDLE*) calloc(NTHREADS,sizeof(HANDLE));
#else
				tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
				bPload_mutex = (pthread_mutex_t*) calloc(NTHREADS,sizeof(pthread_mutex_t));
#endif
				checkpointer((void *)bPload_mutex,__FILE__,"calloc","bPload_mutex" ,__LINE__ -1 );
				bPload_temp_ptr = (struct bPload*) calloc(NTHREADS,sizeof(struct bPload));
				checkpointer((void *)bPload_temp_ptr,__FILE__,"calloc","bPload_temp_ptr" ,__LINE__ -1 );
				bPload_threads_available = (char*) calloc(NTHREADS,sizeof(char));
				checkpointer((void *)bPload_threads_available,__FILE__,"calloc","bPload_threads_available" ,__LINE__ -1 );
				
				memset(bPload_threads_available,1,NTHREADS);
				
				for(j = 0; j < NTHREADS; j++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					bPload_mutex[j] = CreateMutex(NULL, FALSE, NULL);
#else
					pthread_mutex_init(&bPload_mutex[j],NULL);
#endif
				}
				
				do	{
					for(j = 0; j < NTHREADS && !salir; j++)	{

						if(bPload_threads_available[j] && !salir)	{
							bPload_threads_available[j] = 0;
							bPload_temp_ptr[j].from = BASE;
							bPload_temp_ptr[j].threadid = j;
							bPload_temp_ptr[j].finished = 0;
							if( THREADCOUNTER < THREADCYCLES-1)	{
								bPload_temp_ptr[j].to = BASE + THREADBPWORKLOAD;
								bPload_temp_ptr[j].workload = THREADBPWORKLOAD;
							}
							else	{
								bPload_temp_ptr[j].to = BASE + THREADBPWORKLOAD + PERTHREAD_R;
								bPload_temp_ptr[j].workload = THREADBPWORKLOAD + PERTHREAD_R;
								salir = 1;
							}
#if defined(_WIN64) && !defined(__CYGWIN__)
							tid[j] = CreateThread(NULL, 0, thread_bPload_2blooms, (void*) &bPload_temp_ptr[j], 0, &s);
#else
							s = pthread_create(&tid[j],NULL,thread_bPload_2blooms,(void*) &bPload_temp_ptr[j]);
							pthread_detach(tid[j]);
#endif
							BASE+=THREADBPWORKLOAD;
							THREADCOUNTER++;
						}
					}

					if(OLDFINISHED_ITEMS != FINISHED_ITEMS)	{
						printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m2,(int) (((double)FINISHED_ITEMS/(double)bsgs_m2)*100));
						fflush(stdout);
						OLDFINISHED_ITEMS = FINISHED_ITEMS;
					}
					
					for(j = 0 ; j < NTHREADS ; j++)	{

#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(bPload_mutex[j], INFINITE);
						finished = bPload_temp_ptr[j].finished;
						ReleaseMutex(bPload_mutex[j]);
#else
						pthread_mutex_lock(&bPload_mutex[j]);
						finished = bPload_temp_ptr[j].finished;
						pthread_mutex_unlock(&bPload_mutex[j]);
#endif
						if(finished)	{
							bPload_temp_ptr[j].finished = 0;
							bPload_threads_available[j] = 1;
							FINISHED_ITEMS += bPload_temp_ptr[j].workload;
							FINISHED_THREADS_COUNTER++;
						}
					}
				}while(FINISHED_THREADS_COUNTER < THREADCYCLES);
				printf("\r[+] processing %lu/%lu bP points : 100%%     \n",bsgs_m2,bsgs_m2);
				
				free(tid);
				free(bPload_mutex);
				free(bPload_temp_ptr);
				free(bPload_threads_available);
			}
			else{	
				/* We need just to do all the files 
					- first  bllom filter 100% 
					- Second bloom filter 5%
					- third  bloom fitler 0.25 %
					- bp Table 0.25 %
				*/
				FINISHED_THREADS_COUNTER = 0;
				FINISHED_THREADS_BP = 0;
				FINISHED_ITEMS = 0;
				salir = 0;
				BASE = 0;
				THREADCOUNTER = 0;
				if(THREADBPWORKLOAD >= bsgs_m)	{
					THREADBPWORKLOAD = bsgs_m;
				}
				THREADCYCLES = bsgs_m / THREADBPWORKLOAD;
				PERTHREAD_R = bsgs_m % THREADBPWORKLOAD;
				//if(FLAGDEBUG) printf("[D] THREADCYCLES: %lu\n",THREADCYCLES);
				if(PERTHREAD_R != 0)	{
					THREADCYCLES++;
					//if(FLAGDEBUG) printf("[D] PERTHREAD_R: %lu\n",PERTHREAD_R);
				}
				
				printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
				fflush(stdout);
				
#if defined(_WIN64) && !defined(__CYGWIN__)
				tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
				bPload_mutex = (HANDLE*) calloc(NTHREADS,sizeof(HANDLE));
#else
				tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
				bPload_mutex = (pthread_mutex_t*) calloc(NTHREADS,sizeof(pthread_mutex_t));
#endif
				checkpointer((void *)tid,__FILE__,"calloc","tid" ,__LINE__ -1 );
				checkpointer((void *)bPload_mutex,__FILE__,"calloc","bPload_mutex" ,__LINE__ -1 );
				
				bPload_temp_ptr = (struct bPload*) calloc(NTHREADS,sizeof(struct bPload));
				checkpointer((void *)bPload_temp_ptr,__FILE__,"calloc","bPload_temp_ptr" ,__LINE__ -1 );
				bPload_threads_available = (char*) calloc(NTHREADS,sizeof(char));
				checkpointer((void *)bPload_threads_available,__FILE__,"calloc","bPload_threads_available" ,__LINE__ -1 );
				

				memset(bPload_threads_available,1,NTHREADS);
				
				for(j = 0; j < NTHREADS; j++)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					bPload_mutex[j] = CreateMutex(NULL, FALSE, NULL);
#else
					pthread_mutex_init(&bPload_mutex[j],NULL);
#endif
				}
				
				do	{
					for(j = 0; j < NTHREADS && !salir; j++)	{

						if(bPload_threads_available[j] && !salir)	{
							bPload_threads_available[j] = 0;
							bPload_temp_ptr[j].from = BASE;
							bPload_temp_ptr[j].threadid = j;
							bPload_temp_ptr[j].finished = 0;
							if( THREADCOUNTER < THREADCYCLES-1)	{
								bPload_temp_ptr[j].to = BASE + THREADBPWORKLOAD;
								bPload_temp_ptr[j].workload = THREADBPWORKLOAD;
							}
							else	{
								bPload_temp_ptr[j].to = BASE + THREADBPWORKLOAD + PERTHREAD_R;
								bPload_temp_ptr[j].workload = THREADBPWORKLOAD + PERTHREAD_R;
								salir = 1;
								//if(FLAGDEBUG) printf("[D] Salir OK\n");
							}
							//if(FLAGDEBUG) printf("[I] %lu to %lu\n",bPload_temp_ptr[i].from,bPload_temp_ptr[i].to);
#if defined(_WIN64) && !defined(__CYGWIN__)
							tid[j] = CreateThread(NULL, 0, thread_bPload, (void*) &bPload_temp_ptr[j], 0, &s);
#else
							s = pthread_create(&tid[j],NULL,thread_bPload,(void*) &bPload_temp_ptr[j]);
							pthread_detach(tid[j]);
#endif
							BASE+=THREADBPWORKLOAD;
							THREADCOUNTER++;
						}
					}
					if(OLDFINISHED_ITEMS != FINISHED_ITEMS)	{
						printf("\r[+] processing %lu/%lu bP points : %i%%\r",FINISHED_ITEMS,bsgs_m,(int) (((double)FINISHED_ITEMS/(double)bsgs_m)*100));
						fflush(stdout);
						OLDFINISHED_ITEMS = FINISHED_ITEMS;
					}
					
					for(j = 0 ; j < NTHREADS ; j++)	{

#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(bPload_mutex[j], INFINITE);
						finished = bPload_temp_ptr[j].finished;
						ReleaseMutex(bPload_mutex[j]);
#else
						pthread_mutex_lock(&bPload_mutex[j]);
						finished = bPload_temp_ptr[j].finished;
						pthread_mutex_unlock(&bPload_mutex[j]);
#endif
						if(finished)	{
							bPload_temp_ptr[j].finished = 0;
							bPload_threads_available[j] = 1;
							FINISHED_ITEMS += bPload_temp_ptr[j].workload;
							FINISHED_THREADS_COUNTER++;
						}
					}
					
				}while(FINISHED_THREADS_COUNTER < THREADCYCLES);
				printf("\r[+] processing %lu/%lu bP points : 100%%     \n",bsgs_m,bsgs_m);
				
				free(tid);
				free(bPload_mutex);
				free(bPload_temp_ptr);
				free(bPload_threads_available);
			}
		}
		
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE4)	{
			printf("[+] Making checkums .. ");
			fflush(stdout);
		}	
		if(!FLAGREADEDFILE1)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bP[i].bf, bloom_bP[i].bytes,(uint8_t*) bloom_bP_checksums[i].data);
				memcpy(bloom_bP_checksums[i].backup,bloom_bP_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE2)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bPx2nd[i].bf, bloom_bPx2nd[i].bytes,(uint8_t*) bloom_bPx2nd_checksums[i].data);
				memcpy(bloom_bPx2nd_checksums[i].backup,bloom_bPx2nd_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE4)	{
			for(i = 0; i < 256 ; i++)	{
				sha256((uint8_t*)bloom_bPx3rd[i].bf, bloom_bPx3rd[i].bytes,(uint8_t*) bloom_bPx3rd_checksums[i].data);
				memcpy(bloom_bPx3rd_checksums[i].backup,bloom_bPx3rd_checksums[i].data,32);
			}
			printf(".");
		}
		if(!FLAGREADEDFILE1 || !FLAGREADEDFILE2 || !FLAGREADEDFILE4)	{
			printf(" done\n");
			fflush(stdout);
		}	
		if(!FLAGREADEDFILE3)	{
			printf("[+] Sorting %lu elements... ",bsgs_m3);
			fflush(stdout);
			bsgs_sort(bPtable,bsgs_m3);
			sha256((uint8_t*)bPtable, bytes,(uint8_t*) checksum);
			memcpy(checksum_backup,checksum,32);
			printf("Done!\n");
			fflush(stdout);
		}
		if(FLAGSAVEREADFILE || FLAGUPDATEFILE1 )	{
			if(!FLAGREADEDFILE1 || FLAGUPDATEFILE1)	{
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_4_%" PRIu64 ".blm",bsgs_m);
				
				if(FLAGUPDATEFILE1)	{
					printf("[W] Updating old file into a new one\n");
				}
				
				/* Writing file for 1st bloom filter */
				
				fd_aux1 = fopen(buffer_bloom_file,"wb");
				if(fd_aux1 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bP[i],sizeof(struct bloom),1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(bloom_bP[i].bf,bloom_bP[i].bytes,1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(&bloom_bP_checksums[i],sizeof(struct checksumsha256),1,fd_aux1);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
					}
					printf(" Done!\n");
					fclose(fd_aux1);
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
			}
			if(!FLAGREADEDFILE2  )	{
				
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_6_%" PRIu64 ".blm",bsgs_m2);
								
				/* Writing file for 2nd bloom filter */
				fd_aux2 = fopen(buffer_bloom_file,"wb");
				if(fd_aux2 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bPx2nd[i],sizeof(struct bloom),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(bloom_bPx2nd[i].bf,bloom_bPx2nd[i].bytes,1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(&bloom_bPx2nd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
					}
					printf(" Done!\n");
					fclose(fd_aux2);	
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
			}
			
			if(!FLAGREADEDFILE3)	{
				/* Writing file for bPtable */
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_2_%" PRIu64 ".tbl",bsgs_m3);
				fd_aux3 = fopen(buffer_bloom_file,"wb");
				if(fd_aux3 != NULL)	{
					printf("[+] Writing bP Table to file %s .. ",buffer_bloom_file);
					fflush(stdout);
					readed = fwrite(bPtable,bytes,1,fd_aux3);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					readed = fwrite(checksum,32,1,fd_aux3);
					if(readed != 1)	{
						fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
						exit(EXIT_FAILURE);
					}
					printf("Done!\n");
					fclose(fd_aux3);	
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
			}
			if(!FLAGREADEDFILE4)	{
				snprintf(buffer_bloom_file,1024,"keyhunt_bsgs_7_%" PRIu64 ".blm",bsgs_m3);
								
				/* Writing file for 3rd bloom filter */
				fd_aux2 = fopen(buffer_bloom_file,"wb");
				if(fd_aux2 != NULL)	{
					printf("[+] Writing bloom filter to file %s ",buffer_bloom_file);
					fflush(stdout);
					for(i = 0; i < 256;i++)	{
						readed = fwrite(&bloom_bPx3rd[i],sizeof(struct bloom),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(bloom_bPx3rd[i].bf,bloom_bPx3rd[i].bytes,1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						readed = fwrite(&bloom_bPx3rd_checksums[i],sizeof(struct checksumsha256),1,fd_aux2);
						if(readed != 1)	{
							fprintf(stderr,"[E] Error writing the file %s please delete it\n",buffer_bloom_file);
							exit(EXIT_FAILURE);
						}
						if(i % 64 == 0)	{
							printf(".");
							fflush(stdout);
						}
					}
					printf(" Done!\n");
					fclose(fd_aux2);
				}
				else	{
					fprintf(stderr,"[E] Error can't create the file %s\n",buffer_bloom_file);
					exit(EXIT_FAILURE);
				}
			}
		}


		i = 0;

		steps = (uint64_t *) calloc(NTHREADS,sizeof(uint64_t));
		checkpointer((void *)steps,__FILE__,"calloc","steps" ,__LINE__ -1 );
		ends = (unsigned int *) calloc(NTHREADS,sizeof(int));
		checkpointer((void *)ends,__FILE__,"calloc","ends" ,__LINE__ -1 );
#if defined(_WIN64) && !defined(__CYGWIN__)
		tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
#else
		tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
#endif
		checkpointer((void *)tid,__FILE__,"calloc","tid" ,__LINE__ -1 );
		init_search_mode(&n_range_start, &n_range_end);
		
		for(j= 0;j < NTHREADS; j++)	{
			tt = (tothread*) malloc(sizeof(struct tothread));
			checkpointer((void *)tt,__FILE__,"malloc","tt" ,__LINE__ -1 );
			tt->nt = j;
			steps[j] = 0;
			s = 0;
			switch(FLAGBSGSMODE)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				case 0:
					tid[j] = CreateThread(NULL, 0, thread_process_bsgs, (void*)tt, 0, &s);
					break;
				case 1:
					tid[j] = CreateThread(NULL, 0, thread_process_bsgs_backward, (void*)tt, 0, &s);
					break;
				case 2:
					tid[j] = CreateThread(NULL, 0, thread_process_bsgs_both, (void*)tt, 0, &s);
					break;
				case 3:
					tid[j] = CreateThread(NULL, 0, thread_process_bsgs_random, (void*)tt, 0, &s);
					break;
				case 4:
					tid[j] = CreateThread(NULL, 0, thread_process_bsgs_dance, (void*)tt, 0, &s);
					break;
#else
				case 0:
					s = pthread_create(&tid[j],NULL,thread_process_bsgs,(void *)tt);
				break;
				case 1:
					s = pthread_create(&tid[j],NULL,thread_process_bsgs_backward,(void *)tt);
				break;
				case 2:
					s = pthread_create(&tid[j],NULL,thread_process_bsgs_both,(void *)tt);
				break;
				case 3:
					s = pthread_create(&tid[j],NULL,thread_process_bsgs_random,(void *)tt);
				break;
				case 4:
					s = pthread_create(&tid[j],NULL,thread_process_bsgs_dance,(void *)tt);
				break;
#endif
			}
#if defined(_WIN64) && !defined(__CYGWIN__)
			if (tid[j] == NULL) {
#else
			if(s != 0)	{
#endif
				fprintf(stderr,"[E] thread thread_process\n");
				exit(EXIT_FAILURE);
			}
		}
		free(aux);
	}
	if(FLAGMODE != MODE_BSGS)	{
		if(FLAGMODE == MODE_MNEMONIC) {
			if(FLAGMNEMONIC_ALL_LANGS) {
				preload_all_wordlists();
				int loaded = 0;
				for(int i = 0; i < NUM_BIP39_LANGUAGES; i++) {
					if(bip39_all_wordlists[i] != NULL) loaded++;
				}
				if(loaded == 0) {
					fprintf(stderr,"[E] No valid BIP39 wordlists loaded\n");
					exit(EXIT_FAILURE);
				}
				printf("[+] Loaded %d/%d BIP39 language wordlists\n", loaded, NUM_BIP39_LANGUAGES);
			}
			else {
				if(!load_bip39_wordlist(mnemonic_lang_name)) {
					fprintf(stderr,"[E] Failed to load BIP39 wordlist\n");
					exit(EXIT_FAILURE);
				}
			}
			if(FLAGMNEMONIC_WORDS == 0) {
				printf("[+] Mnemonic words: random (12,15,18,21,24)\n");
			} else {
				printf("[+] Mnemonic words: %d\n", FLAGMNEMONIC_WORDS);
			}
		}
		if(FLAGMODE == MODE_POETRY) {
			if(!load_poetry_words("tests/poetry.txt")) {
				fprintf(stderr,"[E] Failed to load poetry wordlist\n");
				exit(EXIT_FAILURE);
			}
		}
		if(FLAGMODE == MODE_BRAINWALLET) {
			if(!load_brainwallet_words("tests/brainwalletwords.txt")) {
				fprintf(stderr,"[E] Failed to load brainwallet wordlist\n");
				exit(EXIT_FAILURE);
			}
		}
		steps = (uint64_t *) calloc(NTHREADS,sizeof(uint64_t));
		checkpointer((void *)steps,__FILE__,"calloc","steps" ,__LINE__ -1 );
		ends = (unsigned int *) calloc(NTHREADS,sizeof(int));
		checkpointer((void *)ends,__FILE__,"calloc","ends" ,__LINE__ -1 );
#if defined(_WIN64) && !defined(__CYGWIN__)
		tid = (HANDLE*)calloc(NTHREADS, sizeof(HANDLE));
#else
		tid = (pthread_t *) calloc(NTHREADS,sizeof(pthread_t));
#endif
		checkpointer((void *)tid,__FILE__,"calloc","tid" ,__LINE__ -1 );
		init_search_mode(&n_range_start, &n_range_end);
		for(j= 0;j < NTHREADS; j++)	{
			tt = (tothread*) malloc(sizeof(struct tothread));
			checkpointer((void *)tt,__FILE__,"malloc","tt" ,__LINE__ -1 );
			tt->nt = j;
			steps[j] = 0;
			s = 0;
			switch(FLAGMODE)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				case MODE_ADDRESS:
				case MODE_RMD160:
					if(FLAGPATH) {
						tid[j] = CreateThread(NULL, 0, thread_process_derived, (void*)tt, 0, &s);
					} else if(FLAGCRYPTO == CRYPTO_TROOT) {
						tid[j] = CreateThread(NULL, 0, thread_process_troot, (void*)tt, 0, &s);
					} else {
						tid[j] = CreateThread(NULL, 0, thread_process, (void*)tt, 0, &s);
					}
				break;
				case MODE_XPOINT:
					tid[j] = CreateThread(NULL, 0, thread_process, (void*)tt, 0, &s);
				break;
				case MODE_MINIKEYS:
					tid[j] = CreateThread(NULL, 0, thread_process_minikeys, (void*)tt, 0, &s);
				break;
				case MODE_VANITY:
					tid[j] = CreateThread(NULL, 0, thread_process_vanity, (void*)tt, 0, &s);
				break;
				case MODE_MNEMONIC:
					tid[j] = CreateThread(NULL, 0, thread_process_mnemonic, (void*)tt, 0, &s);
				break;
				case MODE_POETRY:
					tid[j] = CreateThread(NULL, 0, thread_process_poetry, (void*)tt, 0, &s);
				break;
				case MODE_BRAINWALLET:
					tid[j] = CreateThread(NULL, 0, thread_process_brainwallet, (void*)tt, 0, &s);
				break;
				case MODE_PUB2ADDR:
					tid[j] = CreateThread(NULL, 0, thread_process_pub2addr, (void*)tt, 0, &s);
				break;
#else
				case MODE_ADDRESS:
				case MODE_RMD160:
					if(FLAGPATH) {
						s = pthread_create(&tid[j],NULL,thread_process_derived,(void *)tt);
					} else if(FLAGCRYPTO == CRYPTO_TROOT) {
						s = pthread_create(&tid[j],NULL,thread_process_troot,(void *)tt);
					} else {
						s = pthread_create(&tid[j],NULL,thread_process,(void *)tt);
					}
				break;
				case MODE_XPOINT:
					s = pthread_create(&tid[j],NULL,thread_process,(void *)tt);
				break;
				case MODE_MINIKEYS:
					s = pthread_create(&tid[j],NULL,thread_process_minikeys,(void *)tt);
				break;
				case MODE_VANITY:
					s = pthread_create(&tid[j],NULL,thread_process_vanity,(void *)tt);
				break;
				case MODE_MNEMONIC:
					s = pthread_create(&tid[j],NULL,thread_process_mnemonic,(void *)tt);
				break;
				case MODE_POETRY:
					s = pthread_create(&tid[j],NULL,thread_process_poetry,(void *)tt);
				break;
				case MODE_BRAINWALLET:
					s = pthread_create(&tid[j],NULL,thread_process_brainwallet,(void *)tt);
				break;
				case MODE_PUB2ADDR:
					s = pthread_create(&tid[j],NULL,thread_process_pub2addr,(void *)tt);
				break;
#endif
			}
			if(s != 0)	{
				fprintf(stderr,"[E] pthread_create thread_process\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	for(j =0; j < 7; j++)	{
		int_limits[j].SetBase10((char*)str_limits[j]);
	}
	
	continue_flag = 1;
	total.SetInt32(0);
	pretotal.SetInt32(0);
	debugcount_mpz.Set(&BSGS_N);
	seconds.SetInt32(0);
	do	{
		sleep_ms(1000);
		seconds.AddOne();
		check_flag = 1;
		for(j = 0; j <NTHREADS && check_flag; j++) {
			check_flag &= ends[j];
		}
		if(check_flag)	{
			continue_flag = 0;
		}
		if(OUTPUTSECONDS.IsGreater(&ZERO) ){
			MPZAUX.Set(&seconds);
			MPZAUX.Mod(&OUTPUTSECONDS);
			if(MPZAUX.IsZero()) {
				total.SetInt32(0);
				for(j = 0; j < NTHREADS; j++) {
					pretotal.Set(&debugcount_mpz);
					pretotal.Mult(steps[j]);					
					total.Add(&pretotal);
				}
				
				if(FLAGENDOMORPHISM)	{
					if(FLAGMODE == MODE_XPOINT)	{
						total.Mult(3);
					}
					else	{
						total.Mult(6);
					}
				}
				else	{
					if(FLAGSEARCH == SEARCH_COMPRESS)	{
						total.Mult(2);
					}
				}
				
#ifdef _WIN64
				WaitForSingleObject(bsgs_thread, INFINITE);
#else
				pthread_mutex_lock(&bsgs_thread);
#endif			
				pretotal.Set(&total);
				pretotal.Div(&seconds);
				str_seconds = seconds.GetBase10();
				str_pretotal = pretotal.GetBase10();
				str_total = total.GetBase10();
				
				
			const char *unit = (FLAGMODE == MODE_MNEMONIC || FLAGMODE == MODE_POETRY || FLAGMODE == MODE_BRAINWALLET) ? "mnemonics" : "keys";
			if(pretotal.IsLower(&int_limits[0]))	{
				if(FLAGMATRIX)	{
					sprintf(buffer,"[+] Total %s %s in %s seconds: %s %s/s\n",str_total,unit,str_seconds,str_pretotal,unit);
				}
				else	{
					sprintf(buffer,"\r[+] Total %s %s in %s seconds: %s %s/s\r",str_total,unit,str_seconds,str_pretotal,unit);
				}
			}
			else	{
					i = 0;
					salir = 0;
					while( i < 6 && !salir)	{
						if(pretotal.IsLower(&int_limits[i+1]))	{
							salir = 1;
						}
						else	{
							i++;
						}
					}

					div_pretotal.Set(&pretotal);
					div_pretotal.Div(&int_limits[salir ? i : i-1]);
					str_divpretotal = div_pretotal.GetBase10();
					char scaled_prefix[32];
					const char *raw_prefix = str_limits_prefixs[salir ? i : i-1];
					const char *slash = strstr(raw_prefix, "keys");
					if(slash && strcmp(unit, "mnemonics") == 0) {
						snprintf(scaled_prefix, sizeof(scaled_prefix), "%.*s%s%s",
							(int)(slash - raw_prefix), raw_prefix, unit, slash + 4);
					} else {
						strncpy(scaled_prefix, raw_prefix, sizeof(scaled_prefix)-1);
						scaled_prefix[sizeof(scaled_prefix)-1] = '\0';
					}
					if(FLAGMATRIX)	{
						sprintf(buffer,"[+] Total %s %s in %s seconds: ~%s %s (%s %s/s)\n",str_total,unit,str_seconds,str_divpretotal,scaled_prefix,str_pretotal,unit);
					}
					else	{
						if(THREADOUTPUT == 1)	{
							sprintf(buffer,"\r[+] Total %s %s in %s seconds: ~%s %s (%s %s/s)\r",str_total,unit,str_seconds,str_divpretotal,scaled_prefix,str_pretotal,unit);
						}
						else	{
							sprintf(buffer,"\r[+] Total %s %s in %s seconds: ~%s %s (%s %s/s)\r",str_total,unit,str_seconds,str_divpretotal,scaled_prefix,str_pretotal,unit);
						}
					}
					free(str_divpretotal);

				}
				printf("%s",buffer);
				fflush(stdout);
				THREADOUTPUT = 0;			
#ifdef _WIN64
				ReleaseMutex(bsgs_thread);
#else
				pthread_mutex_unlock(&bsgs_thread);
#endif

				free(str_seconds);
				free(str_pretotal);
				free(str_total);
			}
		}
	}while(continue_flag);
	printf("\nEnd\n");
#ifdef _WIN64
	CloseHandle(write_keys);
	CloseHandle(write_random);
	CloseHandle(bsgs_thread);
#endif
}

void pubkeytopubaddress_dst(char *pkey,int length,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	RMD160Data((const unsigned char*)digest,32, digest+1);
	digest[0] = 0;
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
}

void rmd160toaddress_dst(char *rmd,char *dst){
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = byte_encode_crypto;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
}


char *pubkeytopubaddress(char *pkey,int length)	{
	char *pubaddress = (char*) calloc(MAXLENGTHADDRESS+10,1);
	char *digest = (char*) calloc(60,1);
	size_t pubaddress_size = MAXLENGTHADDRESS+10;
	checkpointer((void *)pubaddress,__FILE__,"malloc","pubaddress" ,__LINE__ -1 );
	checkpointer((void *)digest,__FILE__,"malloc","digest" ,__LINE__ -1 );
	//digest [000...0]
 	sha256((uint8_t*)pkey, length,(uint8_t*) digest);
	//digest [SHA256 32 bytes+000....0]
	RMD160Data((const unsigned char*)digest,32, digest+1);
	//digest [? +RMD160 20 bytes+????000....0]
	digest[0] = 0;
	//digest [0 +RMD160 20 bytes+????000....0]
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	//digest [0 +RMD160 20 bytes+SHA256 32 bytes+....0]
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	//digest [0 +RMD160 20 bytes+SHA256 32 bytes+....0]
	if(!b58enc(pubaddress,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc\n");
	}
	free(digest);
	return pubaddress;	// pubaddress need to be free by te caller funtion
}

int searchbinary(struct address_value *buffer,char *data,int64_t array_length) {
	int64_t half,min,max,current;
	int r = 0,rcmp;
	min = 0;
	current = 0;
	max = array_length;
	half = array_length;
	while(!r && half >= 1) {
		half = (max - min)/2;
		rcmp = memcmp(data,buffer[current+half].value,20);
		if(rcmp == 0)	{
			r = 1;	//Found!!
		}
		else	{
			if(rcmp < 0) { //data < temp_read
				max = (max-half);
			}
			else	{ // data > temp_read
				min = (min+half);
			}
			current = min;
		}
	}
	return r;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_minikeys(LPVOID vargp) {
#else
void *thread_process_minikeys(void *vargp)	{
#endif
	FILE *keys;
	Point publickey[4];
	Int key_mpz[4];
	struct tothread *tt;
	uint64_t count;
	char publickeyhashrmd160_uncompress[4][20];
	char public_key_uncompressed_hex[131];
	char address[4][40],minikey[4][24],minikeys[8][24],buffer_b58[21],minikey2check[24],rawvalue[4][32];
	char *hextemp,*rawbuffer;
	int r,thread_number,continue_flag = 1,k,j,count_valid;
	Int counter;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	rawbuffer = (char*) &counter.bits64;
	count_valid = 0;
	for(k = 0; k < 4; k++)	{
		minikey[k][0] = 'S';
		minikey[k][22] = '?';
		minikey[k][23] = 0x00;
	}
	minikey2check[0] = 'S';
	minikey2check[22] = '?';
	minikey2check[23] = 0x00;
	
	do	{
		if(FLAGRANDOM)	{
			counter.Rand(256);
			for(k = 0; k < 21; k++)	{
				buffer_b58[k] =(uint8_t)((uint8_t) rawbuffer[k] % 58);
			}
		}
		else	{
			if(FLAGBASEMINIKEY)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				memcpy(buffer_b58,raw_baseminikey,21);
				increment_minikey_N(raw_baseminikey);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				memcpy(buffer_b58,raw_baseminikey,21);
				increment_minikey_N(raw_baseminikey);
				pthread_mutex_unlock(&write_random);
#endif
			}
			else	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
#else
				pthread_mutex_lock(&write_random);
#endif
				if(raw_baseminikey == NULL){
					raw_baseminikey = (char *) malloc(22);
					checkpointer((void *)raw_baseminikey,__FILE__,"malloc","raw_baseminikey" ,__LINE__ -1 );
					counter.Rand(256);
					for(k = 0; k < 21; k++)	{
						raw_baseminikey[k] =(uint8_t)((uint8_t) rawbuffer[k] % 58);
					}
					memcpy(buffer_b58,raw_baseminikey,21);
					increment_minikey_N(raw_baseminikey);

				}
				else	{
					memcpy(buffer_b58,raw_baseminikey,21);
					increment_minikey_N(raw_baseminikey);
				}
#if defined(_WIN64) && !defined(__CYGWIN__)				
				ReleaseMutex(write_random);
#else
				pthread_mutex_unlock(&write_random);
#endif
				
			}
		}
		set_minikey(minikey2check+1,buffer_b58,21);
		if(continue_flag)	{
			count = 0;
			if(FLAGMATRIX)	{
					printf("[+] Base minikey: %s     \n",minikey2check);
					fflush(stdout);
			}
			else	{
				if(!FLAGQUIET)	{
					printf("\r[+] Base minikey: %s     \r",minikey2check);
					fflush(stdout);
				}
			}
			do {
				for(j = 0;j<256; j++)	{
					
					if(count_valid > 0)	{
						for(k = 0; k < count_valid ; k++)	{
							memcpy(minikeys[k],minikeys[4+k],22);
						}
					}
					do	{
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[0]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[1]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[2]+1,minikey2check+1,21);
						increment_minikey_index(minikey2check+1,buffer_b58,20);
						memcpy(minikey[3]+1,minikey2check+1,21);
						
						sha256sse_23((uint8_t*)minikey[0],(uint8_t*)minikey[1],(uint8_t*)minikey[2],(uint8_t*)minikey[3],(uint8_t*)rawvalue[0],(uint8_t*)rawvalue[1],(uint8_t*)rawvalue[2],(uint8_t*)rawvalue[3]);
						for(k = 0; k < 4; k++){
							if(rawvalue[k][0] == 0x00)	{
								memcpy(minikeys[count_valid],minikey[k],22);
								count_valid++;
							}
						}
					}while(count_valid < 4);
					count_valid-=4;				
					sha256sse_22((uint8_t*)minikeys[0],(uint8_t*)minikeys[1],(uint8_t*)minikeys[2],(uint8_t*)minikeys[3],(uint8_t*)rawvalue[0],(uint8_t*)rawvalue[1],(uint8_t*)rawvalue[2],(uint8_t*)rawvalue[3]);
					
					for(k = 0; k < 4; k++)	{
						key_mpz[k].Set32Bytes((uint8_t*)rawvalue[k]);
						publickey[k] = secp->ComputePublicKey(&key_mpz[k]);
					}
					
					secp->GetHash160(P2PKH,false,publickey[0],publickey[1],publickey[2],publickey[3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
					
					for(k = 0; k < 4; k++)	{
						r = address_check(publickeyhashrmd160_uncompress[k],20);
						if(r) {
							r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
							if(r) {
								/* hit */
								hextemp = key_mpz[k].GetBase16();
								secp->GetPublicKeyHex(false,publickey[k],public_key_uncompressed_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif
							
								keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
								rmd160toaddress_dst(publickeyhashrmd160_uncompress[k],address[k]);
								minikeys[k][22] = '\0';
								if(keys != NULL)	{
									fprintf(keys,"Private Key: %s\npubkey: %s\nminikey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,minikeys[k],address[k]);
									fclose(keys);
								}
								printf("\nHIT!! Private Key: %s\npubkey: %s\nminikey: %s\naddress: %s\n",hextemp,public_key_uncompressed_hex,minikeys[k],address[k]);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif
								
								free(hextemp);
							}
						}
					}
				}
				steps[thread_number]++;
				count+=1024;
			}while(count < N_SEQUENTIAL_MAX && continue_flag);
		}
	}while(continue_flag);
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_mnemonic(LPVOID vargp) {
#else
void *thread_process_mnemonic(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1, r;
	char mnemonic[512], hex_privkey[128], public_key_hex[256];
	uint8_t privkey[32];
	Point *publickey = (Point*)calloc(1, sizeof(Point));
	publickey->Clear();
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	unsigned int thread_seed = (unsigned int)(time(NULL) ^ (thread_number * 7919));
	int all_lang_counter = thread_number % NUM_BIP39_LANGUAGES;
	
	while(continue_flag) {
		// Cycle through all languages if -L all
		if(FLAGMNEMONIC_ALL_LANGS) {
			if(bip39_all_wordlists[all_lang_counter] == NULL || bip39_all_sizes[all_lang_counter] != 2048) {
				all_lang_counter = (all_lang_counter + 1) % NUM_BIP39_LANGUAGES;
				steps[thread_number]++;
				continue;
			}
			bip39_wordlist = bip39_all_wordlists[all_lang_counter];
			bip39_wordlist_size = bip39_all_sizes[all_lang_counter];
			all_lang_counter = (all_lang_counter + 1) % NUM_BIP39_LANGUAGES;
		}
		
		// Generate random mnemonic
		int word_count = FLAGMNEMONIC_WORDS;
		if(FLAGMNEMONIC_WORDS == 0) {
			int options[] = {12, 15, 18, 21, 24};
			word_count = options[rand_r(&thread_seed) % 5];
		}
		generate_mnemonic(mnemonic, word_count, &thread_seed);
		
		if(mnemonic[0] == '\0' || !validate_mnemonic(mnemonic)) {
			steps[thread_number]++;
			continue;
		}

		// Derive seed from mnemonic using proper BIP-39 (PBKDF2-HMAC-SHA512)
		uint8_t seed[64];
		mnemonic_to_seed(mnemonic, "", seed);
		
		// Derive BIP-32 master key
		uint8_t master_key[32], master_chain[32];
		bip32_master_from_seed(seed, master_key, master_chain);

		// Derivation paths to check
		uint32_t base_paths[3][4] = {
			{0x8000002C, 0x80000000, 0x80000000, 0},  // BIP-44: m/44'/0'/0'/0
			{0x80000031, 0x80000000, 0x80000000, 0},  // BIP-49: m/49'/0'/0'/0
			{0x80000054, 0x80000000, 0x80000000, 0}   // BIP-84: m/84'/0'/0'/0
		};
		int addr_types[3] = {0, 1, 2};  // P2PKH, P2SH-P2WPKH, P2WPKH

		for(int path_idx = 0; path_idx < 3; path_idx++) {
		for(int addr_idx = 0; addr_idx < FLAGDP; addr_idx++) {
			uint8_t derived_key[32], derived_chain[32];
			uint32_t full_path[5];
			memcpy(full_path, base_paths[path_idx], 4 * sizeof(uint32_t));
			full_path[4] = (uint32_t)addr_idx;
			bip32_derive_path(master_key, master_chain, full_path, 5, derived_key, derived_chain);

			Int *key_int = new Int();
			key_int->Set32Bytes(derived_key);
			Point pubKey = secp->ComputePublicKey(key_int);

			if(N > 0) {
				uint8_t addr_hash[20];
				char found_address[64];
				
				if(FLAGMNEMONIC_ETH) {
					generate_binaddress_eth(pubKey, addr_hash);
					snprintf(found_address, sizeof(found_address), "0x");
					for(int i = 0; i < 20; i++) {
						snprintf(found_address + 2 + i*2, 3, "%02x", addr_hash[i]);
					}
				} else {
					compute_address_hash(derived_key, addr_types[path_idx], addr_hash);
					rmd160toaddress_dst((char*)addr_hash, found_address);
				}
				
				r = address_check( addr_hash, 20);
				if(r) {
					r = searchbinary(addressTable, (char*)addr_hash, N);
					if(r) {
						char *hextemp = key_int->GetBase16();
						const char *path_names[] = {"BIP-44 (P2PKH)", "BIP-49 (P2SH-P2WPKH)", "BIP-84 (P2WPKH)"};
						printf("\n[+] MNEMONIC FOUND!\n");
						printf("[+] Derivation: %s index %d\n", path_names[path_idx], addr_idx);
						printf("[+] Path: m/%d'/%d'/%d'/0/%d\n",
							(path_idx == 0 ? 44 : (path_idx == 1 ? 49 : 84)),
							0, 0, addr_idx);
						printf("[+] Mnemonic: %s\n", mnemonic);
						printf("[+] Private Key (hex): %s\n", hextemp);
						if(FLAGMNEMONIC_ETH) {
							printf("[+] ETH Address: %s\n", found_address);
						} else {
							char pubkey_hex[256];
							secp->GetPublicKeyHex(true, pubKey, pubkey_hex);
							printf("[+] Private Key (WIF): ");
							uint8_t wif_buf[34];
							wif_buf[0] = 0x80;
							memcpy(wif_buf + 1, derived_key, 32);
							wif_buf[33] = 0x01;
							uint8_t wif_hash1[32], wif_hash2[32];
							sha256(wif_buf, 34, wif_hash1);
							sha256(wif_hash1, 32, wif_hash2);
							memcpy(wif_buf + 34 - 4, wif_hash2, 4);
							char wif_out[64];
							size_t wif_len = sizeof(wif_out);
							b58enc(wif_out, &wif_len, wif_buf, 34);
							printf("%s\n", wif_out);
							printf("[+] BTC Address: %s\n", found_address);
							printf("[+] Public Key: %s\n", pubkey_hex);
						}
						
#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(write_keys, INFINITE);
#else
						pthread_mutex_lock(&write_keys);
#endif
						FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
						if(f) {
							if(FLAGMNEMONIC_ETH) {
								fprintf(f, "Derivation: %s index %d\nPath: m/%d'/%d'/%d'/0/%d\nMnemonic: %s\nPrivate Key (hex): %s\nETH Address: %s\n\n",
									path_names[path_idx], addr_idx,
									(path_idx == 0 ? 44 : (path_idx == 1 ? 49 : 84)),
									0, 0, addr_idx,
									mnemonic, hextemp, found_address);
							} else {
								fprintf(f, "Derivation: %s index %d\nPath: m/%d'/%d'/%d'/0/%d\nMnemonic: %s\nPrivate Key (hex): %s\nBTC Address: %s\n\n",
									path_names[path_idx], addr_idx,
									(path_idx == 0 ? 44 : (path_idx == 1 ? 49 : 84)),
									0, 0, addr_idx,
									mnemonic, hextemp, found_address);
							}
							fclose(f);
						}
#if defined(_WIN64) && !defined(__CYGWIN__)
						ReleaseMutex(write_keys);
#else
						pthread_mutex_unlock(&write_keys);
#endif
						free(hextemp);
						notify_key_found(key_int);
						delete key_int;
						free(publickey);
						return NULL;
					}
				}
			}
			delete key_int;
		}
		}
		
		steps[thread_number]++;
		if(FLAGQUIET == 0 && steps[thread_number] % 100 == 0) {
			printf("\r[Thread %d] Mnemonic: %s\n", thread_number, mnemonic);
			fflush(stdout);
		}
	}
	free(publickey);
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_derived(LPVOID vargp) {
#else
void *thread_process_derived(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1, r;
	Point publickey;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);

	Int key_mpz;

	while(continue_flag) {
		get_next_search_key(&key_mpz, &n_range_start, &n_range_end);

		uint8_t master_key_raw[32];
		key_mpz.Get32Bytes(master_key_raw);

		uint8_t master_hmac[64];
		hmac_sha512((uint8_t *)"Bitcoin seed", 12, master_key_raw, 32, master_hmac);
		uint8_t master_key[32], master_chain[32];
		memcpy(master_key, master_hmac, 32);
		memcpy(master_chain, master_hmac + 32, 32);

		char *master_hex = key_mpz.GetBase16();

		for(int idx = 0; idx < FLAGDP; idx++) {
			uint32_t full_path[16];
			memcpy(full_path, parsed_path, parsed_path_len * sizeof(uint32_t));
			full_path[parsed_path_len] = (uint32_t)idx;

			uint8_t derived_key[32], derived_chain[32];
			bip32_derive_path(master_key, master_chain, full_path, parsed_path_len + 1, derived_key, derived_chain);

			Int derivedInt;
			derivedInt.Set32Bytes(derived_key);
			publickey = secp->ComputePublicKey(&derivedInt);

			if(FLAGCRYPTO == CRYPTO_BTC) {
				uint8_t addr_hash[20];

				if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH) {
					secp->GetHash160(P2PKH, true, publickey, addr_hash);
					r = address_check(addr_hash, MAXLENGTHADDRESS);
					if(r) {
						r = searchbinary(addressTable, (char*)addr_hash, N);
						if(r) {
							char *hextemp = derivedInt.GetBase16();
							char pubkey_hex[132];
							secp->GetPublicKeyHex(true, publickey, pubkey_hex);
							char address[64];
							rmd160toaddress_dst((char*)addr_hash, address);
							printf("\n[+] ADDRESS FOUND (derived)!\n");
							printf("[+] Full Derivation Path: %s/%d\n", path_string, idx);
							printf("[+] Base Key (hex): %s\n", master_hex);
							printf("[+] Derived Private Key (hex): %s\n", hextemp);
							printf("[+] Public Key: %s\n", pubkey_hex);
							printf("[+] Address: %s\n", address);
							if(FLAGVERBOSE) {
								char chain_hex[65];
								for(int b = 0; b < 32; b++) sprintf(chain_hex + b * 2, "%02x", derived_chain[b]);
								chain_hex[64] = '\0';
								printf("[+] Derived Chain Code: %s\n", chain_hex);
							}
#if defined(_WIN64) && !defined(__CYGWIN__)
							WaitForSingleObject(write_keys, INFINITE);
#else
							pthread_mutex_lock(&write_keys);
#endif
							FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
							if(f) {
								fprintf(f, "Mode: address (derived)\nFull Path: %s/%d\nBase Key: %s\nDerived Private Key: %s\nPublic Key: %s\nAddress: %s\n\n",
									path_string, idx, master_hex, hextemp, pubkey_hex, address);
								fclose(f);
							}
#if defined(_WIN64) && !defined(__CYGWIN__)
							ReleaseMutex(write_keys);
#else
							pthread_mutex_unlock(&write_keys);
#endif
							free(hextemp);
							free(master_hex);
							notify_key_found(&derivedInt);
							return NULL;
						}
					}
				}
				if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH) {
					secp->GetHash160(P2PKH, false, publickey, addr_hash);
					r = address_check(addr_hash, MAXLENGTHADDRESS);
					if(r) {
						r = searchbinary(addressTable, (char*)addr_hash, N);
						if(r) {
							char *hextemp = derivedInt.GetBase16();
							char pubkey_hex[132];
							secp->GetPublicKeyHex(false, publickey, pubkey_hex);
							char address[64];
							rmd160toaddress_dst((char*)addr_hash, address);
							printf("\n[+] ADDRESS FOUND (derived)!\n");
							printf("[+] Full Derivation Path: %s/%d\n", path_string, idx);
							printf("[+] Base Key (hex): %s\n", master_hex);
							printf("[+] Derived Private Key (hex): %s\n", hextemp);
							printf("[+] Public Key: %s\n", pubkey_hex);
							printf("[+] Address: %s\n", address);
							if(FLAGVERBOSE) {
								char chain_hex[65];
								for(int b = 0; b < 32; b++) sprintf(chain_hex + b * 2, "%02x", derived_chain[b]);
								chain_hex[64] = '\0';
								printf("[+] Derived Chain Code: %s\n", chain_hex);
							}
#if defined(_WIN64) && !defined(__CYGWIN__)
							WaitForSingleObject(write_keys, INFINITE);
#else
							pthread_mutex_lock(&write_keys);
#endif
							FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
							if(f) {
								fprintf(f, "Mode: address (derived)\nFull Path: %s/%d\nBase Key: %s\nDerived Private Key: %s\nPublic Key: %s\nAddress: %s\n\n",
									path_string, idx, master_hex, hextemp, pubkey_hex, address);
								fclose(f);
							}
#if defined(_WIN64) && !defined(__CYGWIN__)
							ReleaseMutex(write_keys);
#else
							pthread_mutex_unlock(&write_keys);
#endif
							free(hextemp);
							free(master_hex);
							notify_key_found(&derivedInt);
							return NULL;
						}
					}
				}
			}
			else if(FLAGCRYPTO == CRYPTO_ETH) {
				uint8_t addr_hash[20];
				generate_binaddress_eth(publickey, addr_hash);
				r = address_check(addr_hash, MAXLENGTHADDRESS);
				if(r) {
					r = searchbinary(addressTable, (char*)addr_hash, N);
					if(r) {
						char *hextemp = derivedInt.GetBase16();
						char eth_address[43];
						eth_address[0] = '0'; eth_address[1] = 'x';
						tohex_dst((char*)addr_hash, 20, eth_address + 2);
						printf("\n[+] ETH ADDRESS FOUND (derived)!\n");
						printf("[+] Full Derivation Path: %s/%d\n", path_string, idx);
						printf("[+] Base Key (hex): %s\n", master_hex);
						printf("[+] Derived Private Key (hex): %s\n", hextemp);
						printf("[+] ETH Address: %s\n", eth_address);
						if(FLAGVERBOSE) {
							char chain_hex[65];
							for(int b = 0; b < 32; b++) sprintf(chain_hex + b * 2, "%02x", derived_chain[b]);
							chain_hex[64] = '\0';
							printf("[+] Derived Chain Code: %s\n", chain_hex);
						}
#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(write_keys, INFINITE);
#else
						pthread_mutex_lock(&write_keys);
#endif
						FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
						if(f) {
							fprintf(f, "Mode: address (derived)\nFull Path: %s/%d\nBase Key: %s\nDerived Private Key: %s\nETH Address: %s\n\n",
								path_string, idx, master_hex, hextemp, eth_address);
							fclose(f);
						}
#if defined(_WIN64) && !defined(__CYGWIN__)
						ReleaseMutex(write_keys);
#else
						pthread_mutex_unlock(&write_keys);
#endif
						free(hextemp);
						free(master_hex);
						notify_key_found(&derivedInt);
						return NULL;
					}
				}
			}
		}
		free(master_hex);

		steps[thread_number]++;
		if(FLAGQUIET == 0 && steps[thread_number] % 100 == 0) {
			char *hextemp = key_mpz.GetBase16();
			printf("\r[Thread %d] Base key: %s    \r", thread_number, hextemp);
			free(hextemp);
			fflush(stdout);
		}
	}
	return NULL;
}


void compute_taproot_output(Point &pubkey, uint8_t *x_only_out) {
	uint8_t pubkey_x[32];
	pubkey.x.Get32Bytes(pubkey_x);

	uint8_t tag_hash[32];
	sha256((uint8_t *)"TapTweak", 8, tag_hash);

	uint8_t tag_msg[96];
	memcpy(tag_msg, tag_hash, 32);
	memcpy(tag_msg + 32, tag_hash, 32);
	memcpy(tag_msg + 64, pubkey_x, 32);

	uint8_t tweak_hash[32];
	sha256(tag_msg, 96, tweak_hash);

	Int tweak;
	tweak.Set32Bytes(tweak_hash);

	Point tG = secp->ScalarMultiplication(secp->G, &tweak);
	Point output = secp->Add(pubkey, tG);

	output.x.Get32Bytes(x_only_out);
}

int troot_searchbinary(struct troot_value *arr, uint8_t *data, int64_t array_length) {
	int64_t low = 0, high = array_length - 1, mid;
	while(low <= high) {
		mid = low + (high - low) / 2;
		int cmp = memcmp(arr[mid].value, data, 32);
		if(cmp == 0) return 1;
		if(cmp < 0) low = mid + 1;
		else high = mid - 1;
	}
	return 0;
}

void troot_sort(struct troot_value *arr, int64_t n) {
	for(int64_t i = 1; i < n; i++) {
		struct troot_value key = arr[i];
		int64_t j = i - 1;
		while(j >= 0 && memcmp(arr[j].value, key.value, 32) > 0) {
			arr[j + 1] = arr[j];
			j--;
		}
		arr[j + 1] = key;
	}
}

bool forceReadFileTroot(char *fileName) {
	FILE *f = fopen(fileName, "r");
	if(!f) {
		fprintf(stderr, "[E] Cannot open taproot target file: %s\n", fileName);
		return false;
	}

	uint64_t count = 0;
	char line[128];
	while(fgets(line, sizeof(line), f)) {
		int len = strlen(line);
		while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
		if(len == 64) count++;
	}
	if(count == 0) {
		fprintf(stderr, "[E] No valid 64-char hex keys found in %s\n", fileName);
		fclose(f);
		return false;
	}

	trootTable = (struct troot_value *)malloc(sizeof(struct troot_value) * count);
	if(!trootTable) {
		fprintf(stderr, "[E] Memory allocation failed for troot table\n");
		fclose(f);
		return false;
	}

	fseek(f, 0, SEEK_SET);
	uint64_t i = 0;
	while(i < count && fgets(line, sizeof(line), f)) {
		int len = strlen(line);
		while(len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
		if(len == 64 && isValidHex(line)) {
			hexs2bin(line, trootTable[i].value);
			bloom_add(&troot_bloom, trootTable[i].value, 32);
			bf_add(&troot_bf_filter, trootTable[i].value, 32);
			i++;
		}
	}
	fclose(f);

	N_TROOT = count;
	troot_sort(trootTable, N_TROOT);

	printf("[+] Building taproot binary fuse filter from %" PRIu64 " keys... ", N_TROOT);
	fflush(stdout);
	if(bf_build(&troot_bf_filter) != 0) {
		printf("\n[!] Binary fuse failed, falling back to bloom filter\n");
		troot_bf_filter.use_bloom_fallback = 1;
	} else {
		printf("done! %.2f MB\n", (double)bf_size_in_bytes(&troot_bf_filter) / (double)1048576);
	}
	return true;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_troot(LPVOID vargp) {
#else
void *thread_process_troot(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1;
	Point publickey;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);

	Int key_mpz;

	while(continue_flag) {
		get_next_search_key(&key_mpz, &n_range_start, &n_range_end);

		uint8_t key_bytes[32];
		key_mpz.Get32Bytes(key_bytes);

		for(int i = 0; i < CPU_GRP_SIZE; i++) {
			Int current_key;
			current_key.Set(&key_mpz);
			Int offset;
			offset.SetInt64(i);
			offset.Mult(&stride);
			current_key.Add(&offset);

			publickey = secp->ComputePublicKey(&current_key);

			uint8_t troot_out[32];
			compute_taproot_output(publickey, troot_out);

			int r = bf_check(&troot_bf_filter, troot_out, 32);
			if(r == -1 && troot_bf_filter.use_bloom_fallback) {
				r = bloom_check(&troot_bloom, troot_out, 32);
			}
			if(r) {
				r = troot_searchbinary(trootTable, troot_out, N_TROOT);
				if(r) {
					char *hextemp = current_key.GetBase16();
					char pubkey_hex[132];
					secp->GetPublicKeyHex(true, publickey, pubkey_hex);
					char troot_addr[63];
					troot_addr[0] = 'b'; troot_addr[1] = 'c'; troot_addr[2] = '1'; troot_addr[3] = 'p';
					char x_hex[65];
					for(int b = 0; b < 32; b++) sprintf(x_hex + b * 2, "%02x", troot_out[b]);
					x_hex[64] = '\0';
					printf("\n[+] TAPROOT ADDRESS FOUND!\n");
					printf("[+] Private Key (hex): %s\n", hextemp);
					printf("[+] Public Key: %s\n", pubkey_hex);
					printf("[+] Taproot Output Key (x-only): %s\n", x_hex);
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(write_keys, INFINITE);
#else
					pthread_mutex_lock(&write_keys);
#endif
					FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
					if(f) {
						fprintf(f, "Mode: address (taproot)\nPrivate Key: %s\nPublic Key: %s\nTaproot Output Key: %s\n\n",
							hextemp, pubkey_hex, x_hex);
						fclose(f);
					}
#if defined(_WIN64) && !defined(__CYGWIN__)
					ReleaseMutex(write_keys);
#else
					pthread_mutex_unlock(&write_keys);
#endif
					free(hextemp);
					notify_key_found(&current_key);
					return NULL;
				}
			}
		}

		Int grp_stride;
		grp_stride.SetInt64(CPU_GRP_SIZE);
		grp_stride.Mult(&stride);
		key_mpz.Add(&grp_stride);

		steps[thread_number]++;
		if(FLAGQUIET == 0 && steps[thread_number] % 100 == 0) {
			char *hextemp = key_mpz.GetBase16();
			printf("\r[Thread %d] Base key: %s    \r", thread_number, hextemp);
			free(hextemp);
			fflush(stdout);
		}
	}
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_poetry(LPVOID vargp) {
#else
void *thread_process_poetry(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1;
	char poetry[2048], public_key_hex[131];
	uint8_t privkey[32];
	Point publickey;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	unsigned int thread_seed = (unsigned int)(time(NULL) ^ (thread_number * 7919));
	
	while(continue_flag) {
		// Generate random poetry
		int word_count = FLAGPOETRY_WORDS;
		if(FLAGPOETRY_WORDS == 0) {
			if(FLAGBITRANGE) {
				// Match poetry words to bit range: 3 words = 32 bits
				word_count = ((bitrange + 31) / 32) * 3;
				if(word_count < 3) word_count = 3;
				if(word_count > 24) word_count = 24;
			} else {
				word_count = 12;  // default 128 bits
			}
		}
		generate_poetry(poetry, word_count, &thread_seed);
		
		// Decode poetry to hex
		char hex_decoded[256] = {0};
		poetry_to_hex(poetry, hex_decoded);
		
		// Pad hex to 64 chars (32 bytes) with leading zeros
		int hex_len = strlen(hex_decoded);
		if(hex_len > 0 && hex_len < 64) {
			char padded[65] = {0};
			int zeros = 64 - hex_len;
			memset(padded, '0', zeros);
			memcpy(padded + zeros, hex_decoded, hex_len);
			memcpy(hex_decoded, padded, 64);
			hex_len = 64;
		}
		
		// Convert hex to private key
		if(hex_len >= 64) {
			hex_decoded[64] = '\0';
			for(int i = 0; i < 32; i++) {
				unsigned int byte;
				sscanf(hex_decoded + i * 2, "%2x", &byte);
				privkey[i] = (uint8_t)byte;
			}
			
			// Compute public key and address
			Int key_int;
			key_int.Set32Bytes(privkey);
			publickey = secp->ComputePublicKey(&key_int);
			
			// Get compressed public key
			secp->GetPublicKeyHex(true, publickey, public_key_hex);
			
			// Check against address list
			if(N > 0) {
				// Get hash160 of public key
				uint8_t rmd160hash[20];
				secp->GetHash160(P2PKH, true, publickey, rmd160hash);
				
				int r = address_check( rmd160hash, 20);
				if(r) {
					r = searchbinary(addressTable, (char*)rmd160hash, N);
					if(r) {
						// FOUND!
						char *hextemp = key_int.GetBase16();
						printf("\n[+] POETRY FOUND!\n");
						printf("[+] Poetry: %s\n", poetry);
						printf("[+] Private Key (hex): %s\n", hextemp);
						printf("[+] Public Key: %s\n", public_key_hex);
						
#if defined(_WIN64) && !defined(__CYGWIN__)
						WaitForSingleObject(write_keys, INFINITE);
#else
						pthread_mutex_lock(&write_keys);
#endif
						FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
						if(f) {
							fprintf(f, "Poetry: %s\nPrivate Key: %s\nPublic Key: %s\n\n", poetry, hextemp, public_key_hex);
							fclose(f);
						}
#if defined(_WIN64) && !defined(__CYGWIN__)
						ReleaseMutex(write_keys);
#else
						pthread_mutex_unlock(&write_keys);
#endif
						free(hextemp);
						notify_key_found(&key_int);
					}
				}
			}
		}
		
		steps[thread_number]++;
		if(FLAGQUIET == 0 && steps[thread_number] % 100 == 0) {
			printf("\r[Thread %d] Poetry: %.30s... ", thread_number, poetry);
			fflush(stdout);
		}
	}
	return NULL;
}


// Brainwallet helper: leet speak mutations
void brainwallet_leet(const char *input, char *output) {
	int j = 0;
	for(int i = 0; input[i] && j < 500; i++) {
		char c = input[i];
		if(c == 'a' || c == 'A') { output[j++] = '@'; }
		else if(c == 'e' || c == 'E') { output[j++] = '3'; }
		else if(c == 'i' || c == 'I') { output[j++] = '!'; }
		else if(c == 'o' || c == 'O') { output[j++] = '0'; }
		else if(c == 's' || c == 'S') { output[j++] = '$'; }
		else if(c == 't' || c == 'T') { output[j++] = '7'; }
		else if(c == 'l' || c == 'L') { output[j++] = '1'; }
		else if(c == 'g' || c == 'G') { output[j++] = '9'; }
		else { output[j++] = c; }
	}
	output[j] = '\0';
}

// Brainwallet helper: capitalize first letter
void brainwallet_capitalize(const char *input, char *output) {
	strcpy(output, input);
	if(output[0] >= 'a' && output[0] <= 'z') {
		output[0] = output[0] - 32;
	}
}

// Brainwallet helper: all caps
void brainwallet_upper(const char *input, char *output) {
	int i;
	for(i = 0; input[i] && i < 500; i++) {
		if(input[i] >= 'a' && input[i] <= 'z') {
			output[i] = input[i] - 32;
		} else {
			output[i] = input[i];
		}
	}
	output[i] = '\0';
}

// Brainwallet: check a passphrase against targets
static int brainwallet_check(const char *passphrase, uint8_t *privkey_data, int hash_mode, int thread_number) {
	uint8_t privkey[32];
	Point publickey;
	char public_key_hex[256];

	if(hash_mode == 0) {
		sha256((uint8_t *)passphrase, strlen(passphrase), privkey);
	} else {
		uint8_t hash1[32];
		sha256((uint8_t *)passphrase, strlen(passphrase), hash1);
		sha256(hash1, 32, privkey);
	}

	Int key_int;
	key_int.Set32Bytes(privkey);
	publickey = secp->ComputePublicKey(&key_int);
	secp->GetPublicKeyHex(true, publickey, public_key_hex);

	if(N > 0) {
		uint8_t rmd160hash[20];
		secp->GetHash160(P2PKH, true, publickey, rmd160hash);

		int r = address_check( rmd160hash, 20);
		if(r) {
			r = searchbinary(addressTable, (char*)rmd160hash, N);
			if(r) {
				char *hextemp = key_int.GetBase16();
				char address[64];
				rmd160toaddress_dst((char*)rmd160hash, address);
				printf("\n[+] BRAINWALLET FOUND!\n");
				printf("[+] Passphrase: %s\n", passphrase);
				printf("[+] Hash mode: %s\n", hash_mode == 0 ? "SHA256" : "SHA256(SHA256())");
				printf("[+] Private Key (hex): %s\n", hextemp);
				printf("[+] Private Key (WIF): ");
				uint8_t wif_buf[34];
				wif_buf[0] = 0x80;
				memcpy(wif_buf + 1, privkey, 32);
				wif_buf[33] = 0x01;
				uint8_t wif_hash1[32], wif_hash2[32];
				sha256(wif_buf, 34, wif_hash1);
				sha256(wif_hash1, 32, wif_hash2);
				memcpy(wif_buf + 34 - 4, wif_hash2, 4);
				char wif_out[64];
				size_t wif_len = sizeof(wif_out);
				b58enc(wif_out, &wif_len, wif_buf, 34);
				printf("%s\n", wif_out);
				printf("[+] BTC Address: %s\n", address);
				printf("[+] Public Key: %s\n", public_key_hex);

#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_keys, INFINITE);
#else
				pthread_mutex_lock(&write_keys);
#endif
				FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
				if(f) {
					fprintf(f, "Brainwallet Passphrase: %s\nHash mode: %s\nPrivate Key (hex): %s\nBTC Address: %s\nPublic Key: %s\n\n",
						passphrase, hash_mode == 0 ? "SHA256" : "SHA256(SHA256())", hextemp, address, public_key_hex);
					fclose(f);
				}
#if defined(_WIN64) && !defined(__CYGWIN__)
				ReleaseMutex(write_keys);
#else
				pthread_mutex_unlock(&write_keys);
#endif
				free(hextemp);
				notify_key_found(&key_int);
				return 1;
			}
		}
	}
	return 0;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_brainwallet(LPVOID vargp) {
#else
void *thread_process_brainwallet(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1;
	char passphrase[512], variant[512];
	uint8_t privkey[32];
	unsigned int thread_seed;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	thread_seed = (unsigned int)(time(NULL) ^ (thread_number * 7919));

	const char *separators[] = {" ", "-", "_", ".", ""};
	const char *numbers[] = {"", "1", "123", "0", "2024", "69", "420", "7", "13", "99"};
	int num_separators = 5;
	int num_numbers = 10;

	while(continue_flag) {
		int word_count = FLAGBRAINWALLET_WORDS;
		if(FLAGBRAINWALLET_WORDS == 0) {
			int options[] = {1, 2, 3, 6, 9, 12};
			word_count = options[rand_r(&thread_seed) % 6];
		}

		// Build passphrase from N random words
		passphrase[0] = '\0';
		int sep_idx = rand_r(&thread_seed) % num_separators;
		const char *sep = separators[sep_idx];

		for(int w = 0; w < word_count; w++) {
			int wi = rand_r(&thread_seed) % brainwallet_words_size;
			if(w > 0) strcat(passphrase, sep);
			strcat(passphrase, brainwallet_words[wi]);
		}

		// Test original passphrase - SHA256
		if(brainwallet_check(passphrase, privkey, 0, thread_number)) return NULL;
		steps[thread_number]++;

		// Test double SHA256
		if(brainwallet_check(passphrase, privkey, 1, thread_number)) return NULL;
		steps[thread_number]++;

		// Test with number suffix
		for(int n = 0; n < num_numbers; n++) {
			if(strlen(numbers[n]) > 0) {
				snprintf(variant, sizeof(variant), "%s%s", passphrase, numbers[n]);
				if(brainwallet_check(variant, privkey, 0, thread_number)) return NULL;
				steps[thread_number]++;
			}
		}

		// Test leet speak
		brainwallet_leet(passphrase, variant);
		if(strcmp(variant, passphrase) != 0) {
			if(brainwallet_check(variant, privkey, 0, thread_number)) return NULL;
			steps[thread_number]++;
		}

		// Test capitalize
		brainwallet_capitalize(passphrase, variant);
		if(strcmp(variant, passphrase) != 0) {
			if(brainwallet_check(variant, privkey, 0, thread_number)) return NULL;
			steps[thread_number]++;
		}

		// Test all caps
		brainwallet_upper(passphrase, variant);
		if(strcmp(variant, passphrase) != 0) {
			if(brainwallet_check(variant, privkey, 0, thread_number)) return NULL;
			steps[thread_number]++;
		}

		if(FLAGQUIET == 0 && steps[thread_number] % 1000 == 0) {
			printf("\r[Thread %d] Brainwallet: %s (%d words, %llu tested)... ",
				thread_number, passphrase, word_count, (unsigned long long)steps[thread_number]);
			fflush(stdout);
		}
	}
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_pub2addr(LPVOID vargp) {
#else
void *thread_process_pub2addr(void *vargp) {
#endif
	struct tothread *tt;
	int thread_number, continue_flag = 1, r;
	Point publickey;
	uint8_t addr_hash[20];
	char found_address[64];
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	unsigned int thread_seed = (unsigned int)(time(NULL) ^ (thread_number * 7919));
	Int key_mpz;

	while(continue_flag) {
		key_mpz.Rand(&n_range_start, &n_range_end);

		publickey = secp->ComputePublicKey(&key_mpz);

		if(FLAGCRYPTO == CRYPTO_ETH) {
			generate_binaddress_eth(publickey, addr_hash);
			snprintf(found_address, sizeof(found_address), "0x");
			for(int i = 0; i < 20; i++) {
				snprintf(found_address + 2 + i*2, 3, "%02x", addr_hash[i]);
			}
		}
		else {
			secp->GetHash160(P2PKH, true, publickey, addr_hash);
			rmd160toaddress_dst((char*)addr_hash, found_address);
		}

		r = address_check( addr_hash, 20);
		if(r) {
			r = searchbinary(addressTable, (char*)addr_hash, N);
			if(r) {
				char *hextemp = key_mpz.GetBase16();
				printf("\n[+] PUBKEY2ADDR FOUND!\n");
				printf("[+] Private Key (hex): %s\n", hextemp);
				if(FLAGCRYPTO == CRYPTO_ETH) {
					printf("[+] ETH Address: %s\n", found_address);
				}
				else {
					char pubkey_hex[256];
					secp->GetPublicKeyHex(true, publickey, pubkey_hex);
					printf("[+] Public Key: %s\n", pubkey_hex);
					printf("[+] BTC Address: %s\n", found_address);
				}
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_keys, INFINITE);
#else
				pthread_mutex_lock(&write_keys);
#endif
				FILE *f = fopen("KEYFOUNDKEYFOUND.txt", "a");
				if(f) {
					fprintf(f, "Private Key: %s\nAddress: %s\n", hextemp, found_address);
					fclose(f);
				}
#if defined(_WIN64) && !defined(__CYGWIN__)
				ReleaseMutex(write_keys);
#else
				pthread_mutex_unlock(&write_keys);
#endif
				free(hextemp);
				notify_key_found(&key_mpz);
				return NULL;
			}
		}

		steps[thread_number]++;
		if(FLAGQUIET == 0 && steps[thread_number] % 100 == 0) {
			char *hextemp = key_mpz.GetBase16();
			printf("\r[Thread %d] Key: %s -> %s     \r", thread_number, hextemp, found_address);
			fflush(stdout);
			free(hextemp);
		}
	}
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process(LPVOID vargp) {
#else
void *thread_process(void *vargp)	{
#endif
	struct tothread *tt;
	Point pts[CPU_GRP_SIZE];
	Point endomorphism_beta[CPU_GRP_SIZE];
	Point endomorphism_beta2[CPU_GRP_SIZE];
	Point endomorphism_negeted_point[4];
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Point pp;
	Point pn;
	int i,l,pp_offset,pn_offset,hLength = (CPU_GRP_SIZE / 2 - 1);
	uint64_t j,count;
	Point R,temporal,publickey;
	int r,thread_number,continue_flag = 1,k;
	char *hextemp = NULL;
	
	char publickeyhashrmd160[20];
	char publickeyhashrmd160_uncompress[4][20];
	char rawvalue[32];
	
	char publickeyhashrmd160_endomorphism[12][4][20];
	
	bool calculate_y = FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH || FLAGCRYPTO  == CRYPTO_ETH;
	Int key_mpz,keyfound,temp_stride;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	grp->Set(dx);
			
	do {
		if(FLAGSEARCHMODE == SEARCHMODE_SEQUENTIAL) {
			if(n_range_start.IsLower(&n_range_end))	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SEQUENTIAL_MAX);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SEQUENTIAL_MAX);
				pthread_mutex_unlock(&write_random);
#endif
			}
			else	{
				continue_flag = 0;
			}
		}
		else	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			WaitForSingleObject(write_random, INFINITE);
			get_next_search_key(&key_mpz, &n_range_start, &n_range_end);
			ReleaseMutex(write_random);
#else
			pthread_mutex_lock(&write_random);
			get_next_search_key(&key_mpz, &n_range_start, &n_range_end);
			pthread_mutex_unlock(&write_random);
#endif
		}
		if(continue_flag)	{
			count = 0;
			if(FLAGMATRIX)	{
					hextemp = key_mpz.GetBase16();
					printf("Base key: %s thread %i\n",hextemp,thread_number);
					fflush(stdout);
					free(hextemp);
			}
			else	{
				if(FLAGQUIET == 0){
					hextemp = key_mpz.GetBase16();
					printf("\rBase key: %s     \r",hextemp);
					fflush(stdout);
					free(hextemp);
					THREADOUTPUT = 1;
				}
			}
			do {
				temp_stride.SetInt32(CPU_GRP_SIZE / 2);
				temp_stride.Mult(&stride);
				key_mpz.Add(&temp_stride);
	 			startP = secp->ComputePublicKey(&key_mpz);
				key_mpz.Sub(&temp_stride);

				for(i = 0; i < hLength; i++) {
					dx[i].ModSub(&Gn[i].x,&startP.x);
				}
			
				dx[i].ModSub(&Gn[i].x,&startP.x);  // For the first point
				dx[i + 1].ModSub(&_2Gn.x,&startP.x); // For the next center point
				grp->ModInv();

				pts[CPU_GRP_SIZE / 2] = startP;

				for(i = 0; i<hLength; i++) {
					pp = startP;
					pn = startP;

					// P = startP + i*G
					dy.ModSub(&Gn[i].y,&pp.y);

					_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

					if(calculate_y)	{
						pp.y.ModSub(&Gn[i].x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
					}

					// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
					dyn.Set(&Gn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)
					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

					if(calculate_y)	{
						pn.y.ModSub(&Gn[i].x,&pn.x);
						pn.y.ModMulK1(&_s);
						pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
					}

					pp_offset = CPU_GRP_SIZE / 2 + (i + 1);
					pn_offset = CPU_GRP_SIZE / 2 - (i + 1);

					pts[pp_offset] = pp;
					pts[pn_offset] = pn;
					
					if(FLAGENDOMORPHISM)	{
						/*
							Q = (x,y)
							For any point Q
							Q*lambda = (x*beta mod p ,y)
							Q*lambda is a Scalar Multiplication
							x*beta is just a Multiplication (Very fast)
						*/
						
						if( calculate_y  )	{
							endomorphism_beta[pp_offset].y.Set(&pp.y);
							endomorphism_beta[pn_offset].y.Set(&pn.y);
							endomorphism_beta2[pp_offset].y.Set(&pp.y);
							endomorphism_beta2[pn_offset].y.Set(&pn.y);
						}
						endomorphism_beta[pp_offset].x.ModMulK1(&pp.x, &beta);
						endomorphism_beta[pn_offset].x.ModMulK1(&pn.x, &beta);
						endomorphism_beta2[pp_offset].x.ModMulK1(&pp.x, &beta2);
						endomorphism_beta2[pn_offset].x.ModMulK1(&pn.x, &beta2);
					}
				}
				/*
					Half point for endomorphism because pts[CPU_GRP_SIZE / 2] was not calcualte in the previous cycle
				*/
				if(FLAGENDOMORPHISM)	{
					if( calculate_y  )	{

						endomorphism_beta[CPU_GRP_SIZE / 2].y.Set(&pts[CPU_GRP_SIZE / 2].y);
						endomorphism_beta2[CPU_GRP_SIZE / 2].y.Set(&pts[CPU_GRP_SIZE / 2].y);
					}
					endomorphism_beta[CPU_GRP_SIZE / 2].x.ModMulK1(&pts[CPU_GRP_SIZE / 2].x, &beta);
					endomorphism_beta2[CPU_GRP_SIZE / 2].x.ModMulK1(&pts[CPU_GRP_SIZE / 2].x, &beta2);
				}

				// First point (startP - (GRP_SZIE/2)*G)
				pn = startP;
				dyn.Set(&Gn[i].y);
				dyn.ModNeg();
				dyn.ModSub(&pn.y);

				_s.ModMulK1(&dyn,&dx[i]);
				_p.ModSquareK1(&_s);

				pn.x.ModNeg();
				pn.x.ModAdd(&_p);
				pn.x.ModSub(&Gn[i].x);
				
				if(calculate_y)	{
					pn.y.ModSub(&Gn[i].x,&pn.x);
					pn.y.ModMulK1(&_s);
					pn.y.ModAdd(&Gn[i].y);
				}

				pts[0] = pn;
				
				/*
					First point for endomorphism because pts[0] was not calcualte previously
				*/
				if(FLAGENDOMORPHISM)	{
					if( calculate_y  )	{
						endomorphism_beta[0].y.Set(&pn.y);
						endomorphism_beta2[0].y.Set(&pn.y);
					}
					endomorphism_beta[0].x.ModMulK1(&pn.x, &beta);
					endomorphism_beta2[0].x.ModMulK1(&pn.x, &beta2);
				}
								
				for(j = 0; j < CPU_GRP_SIZE/4;j++){
					switch(FLAGMODE)	{
						case MODE_RMD160:
						case MODE_ADDRESS:
							if(FLAGCRYPTO == CRYPTO_BTC || FLAGCRYPTO == CRYPTO_LTC || FLAGCRYPTO == CRYPTO_BTG || FLAGCRYPTO == CRYPTO_BCH || FLAGCRYPTO == CRYPTO_DOGE || FLAGCRYPTO == CRYPTO_XRP || FLAGCRYPTO == CRYPTO_ALL){

								if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH ){
									if(FLAGENDOMORPHISM)	{
										secp->GetHash160_fromX(P2PKH,0x02,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[0][0],(uint8_t*)publickeyhashrmd160_endomorphism[0][1],(uint8_t*)publickeyhashrmd160_endomorphism[0][2],(uint8_t*)publickeyhashrmd160_endomorphism[0][3]);
										secp->GetHash160_fromX(P2PKH,0x03,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[1][0],(uint8_t*)publickeyhashrmd160_endomorphism[1][1],(uint8_t*)publickeyhashrmd160_endomorphism[1][2],(uint8_t*)publickeyhashrmd160_endomorphism[1][3]);

										secp->GetHash160_fromX(P2PKH,0x02,&endomorphism_beta[(j*4)].x,&endomorphism_beta[(j*4)+1].x,&endomorphism_beta[(j*4)+2].x,&endomorphism_beta[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[2][0],(uint8_t*)publickeyhashrmd160_endomorphism[2][1],(uint8_t*)publickeyhashrmd160_endomorphism[2][2],(uint8_t*)publickeyhashrmd160_endomorphism[2][3]);
										secp->GetHash160_fromX(P2PKH,0x03,&endomorphism_beta[(j*4)].x,&endomorphism_beta[(j*4)+1].x,&endomorphism_beta[(j*4)+2].x,&endomorphism_beta[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[3][0],(uint8_t*)publickeyhashrmd160_endomorphism[3][1],(uint8_t*)publickeyhashrmd160_endomorphism[3][2],(uint8_t*)publickeyhashrmd160_endomorphism[3][3]);

										secp->GetHash160_fromX(P2PKH,0x02,&endomorphism_beta2[(j*4)].x,&endomorphism_beta2[(j*4)+1].x,&endomorphism_beta2[(j*4)+2].x,&endomorphism_beta2[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[4][0],(uint8_t*)publickeyhashrmd160_endomorphism[4][1],(uint8_t*)publickeyhashrmd160_endomorphism[4][2],(uint8_t*)publickeyhashrmd160_endomorphism[4][3]);
										secp->GetHash160_fromX(P2PKH,0x03,&endomorphism_beta2[(j*4)].x,&endomorphism_beta2[(j*4)+1].x,&endomorphism_beta2[(j*4)+2].x,&endomorphism_beta2[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[5][0],(uint8_t*)publickeyhashrmd160_endomorphism[5][1],(uint8_t*)publickeyhashrmd160_endomorphism[5][2],(uint8_t*)publickeyhashrmd160_endomorphism[5][3]);
									}
									else	{
										secp->GetHash160_fromX(P2PKH,0x02,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[0][0],(uint8_t*)publickeyhashrmd160_endomorphism[0][1],(uint8_t*)publickeyhashrmd160_endomorphism[0][2],(uint8_t*)publickeyhashrmd160_endomorphism[0][3]);
										secp->GetHash160_fromX(P2PKH,0x03,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[1][0],(uint8_t*)publickeyhashrmd160_endomorphism[1][1],(uint8_t*)publickeyhashrmd160_endomorphism[1][2],(uint8_t*)publickeyhashrmd160_endomorphism[1][3]);
									}
									
								}
								if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH){
									if(FLAGENDOMORPHISM)	{
										for(l = 0; l < 4; l++)	{
											endomorphism_negeted_point[l] = secp->Negation(pts[(j*4)+l]);
										}
										secp->GetHash160(P2PKH,false, pts[(j*4)], pts[(j*4)+1], pts[(j*4)+2], pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_endomorphism[6][0],(uint8_t*)publickeyhashrmd160_endomorphism[6][1],(uint8_t*)publickeyhashrmd160_endomorphism[6][2],(uint8_t*)publickeyhashrmd160_endomorphism[6][3]);
										secp->GetHash160(P2PKH,false,endomorphism_negeted_point[0] ,endomorphism_negeted_point[1],endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[7][0],(uint8_t*)publickeyhashrmd160_endomorphism[7][1],(uint8_t*)publickeyhashrmd160_endomorphism[7][2],(uint8_t*)publickeyhashrmd160_endomorphism[7][3]);
										for(l = 0; l < 4; l++)	{
											endomorphism_negeted_point[l] = secp->Negation(endomorphism_beta[(j*4)+l]);
										}
										secp->GetHash160(P2PKH,false,endomorphism_beta[(j*4)],  endomorphism_beta[(j*4)+1], endomorphism_beta[(j*4)+2], endomorphism_beta[(j*4)+3] ,(uint8_t*)publickeyhashrmd160_endomorphism[8][0],(uint8_t*)publickeyhashrmd160_endomorphism[8][1],(uint8_t*)publickeyhashrmd160_endomorphism[8][2],(uint8_t*)publickeyhashrmd160_endomorphism[8][3]);
										secp->GetHash160(P2PKH,false,endomorphism_negeted_point[0],endomorphism_negeted_point[1],endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[9][0],(uint8_t*)publickeyhashrmd160_endomorphism[9][1],(uint8_t*)publickeyhashrmd160_endomorphism[9][2],(uint8_t*)publickeyhashrmd160_endomorphism[9][3]);

										for(l = 0; l < 4; l++)	{
											endomorphism_negeted_point[l] = secp->Negation(endomorphism_beta2[(j*4)+l]);
										}
										secp->GetHash160(P2PKH,false, endomorphism_beta2[(j*4)],  endomorphism_beta2[(j*4)+1] ,  endomorphism_beta2[(j*4)+2] ,  endomorphism_beta2[(j*4)+3] ,(uint8_t*)publickeyhashrmd160_endomorphism[10][0],(uint8_t*)publickeyhashrmd160_endomorphism[10][1],(uint8_t*)publickeyhashrmd160_endomorphism[10][2],(uint8_t*)publickeyhashrmd160_endomorphism[10][3]);
										secp->GetHash160(P2PKH,false, endomorphism_negeted_point[0], endomorphism_negeted_point[1],   endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[11][0],(uint8_t*)publickeyhashrmd160_endomorphism[11][1],(uint8_t*)publickeyhashrmd160_endomorphism[11][2],(uint8_t*)publickeyhashrmd160_endomorphism[11][3]);

									}
									else	{
										secp->GetHash160(P2PKH,false,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
										
									}
								}
							}								
							else if(FLAGCRYPTO == CRYPTO_ETH){
								if(FLAGENDOMORPHISM)	{
									for(k = 0; k < 4;k++)	{
										endomorphism_negeted_point[k] = secp->Negation(pts[(j*4)+k]);
										generate_binaddress_eth(pts[(4*j)+k],(uint8_t*)publickeyhashrmd160_endomorphism[0][k]);
										generate_binaddress_eth(endomorphism_negeted_point[k],(uint8_t*)publickeyhashrmd160_endomorphism[1][k]);
										endomorphism_negeted_point[k] = secp->Negation(endomorphism_beta[(j*4)+k]);
										generate_binaddress_eth(endomorphism_beta[(4*j)+k],(uint8_t*)publickeyhashrmd160_endomorphism[2][k]);
										generate_binaddress_eth(endomorphism_negeted_point[k],(uint8_t*)publickeyhashrmd160_endomorphism[3][k]);
										endomorphism_negeted_point[k] = secp->Negation(endomorphism_beta2[(j*4)+k]);
										generate_binaddress_eth(endomorphism_beta[(4*j)+k],(uint8_t*)publickeyhashrmd160_endomorphism[4][k]);
										generate_binaddress_eth(endomorphism_negeted_point[k],(uint8_t*)publickeyhashrmd160_endomorphism[5][k]);
									}
								}
								else	{
									for(k = 0; k < 4;k++)	{
										generate_binaddress_eth(pts[(4*j)+k],(uint8_t*)publickeyhashrmd160_uncompress[k]);
									}
								}
								
							}
						break;
					}


					switch(FLAGMODE)	{
						case MODE_RMD160:
						case MODE_ADDRESS:
							if( FLAGCRYPTO  == CRYPTO_BTC) {
								
								for(k = 0; k < 4;k++)	{
									if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH){
										if(FLAGENDOMORPHISM)	{
											for(l = 0;l < 6; l++)	{
												r = address_check(publickeyhashrmd160_endomorphism[l][k],MAXLENGTHADDRESS);
												if(r) {
													r = searchbinary(addressTable,publickeyhashrmd160_endomorphism[l][k],N);
													if(r) {
														keyfound.SetInt32(k);
														keyfound.Mult(&stride);
														keyfound.Add(&key_mpz);
														publickey = secp->ComputePublicKey(&keyfound);
														switch(l)	{
															case 0:	//Original point, prefix 02
																if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 02
															break;
															case 1:	//Original point, prefix 03
																if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 03
															break;
															case 2:	//Beta point, prefix 02
																keyfound.ModMulK1order(&lambda);
																if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 02
															break;
															case 3:	//Beta point, prefix 03											
																keyfound.ModMulK1order(&lambda);
																if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 02
															break;
															case 4:	//Beta^2 point, prefix 02
																keyfound.ModMulK1order(&lambda2);
																if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 02
															break;
															case 5:	//Beta^2 point, prefix 03
																keyfound.ModMulK1order(&lambda2);
																if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
																// else we dont need to chage the current keyfound because it already have prefix 02
															break;
														}
														writekey(true,&keyfound);
														notify_key_found(&keyfound);
													}
												}
											}
										}
										else	{
											for(l = 0;l < 2; l++)	{
												r = address_check(publickeyhashrmd160_endomorphism[l][k],MAXLENGTHADDRESS);
												if(r) {
													r = searchbinary(addressTable,publickeyhashrmd160_endomorphism[l][k],N);
													if(r) {
														keyfound.SetInt32(k);
														keyfound.Mult(&stride);
														keyfound.Add(&key_mpz);
														
														publickey = secp->ComputePublicKey(&keyfound);
														secp->GetHash160(P2PKH,true,publickey,(uint8_t*)publickeyhashrmd160);
														if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160,20) != 0)	{
															keyfound.Neg();
															keyfound.Add(&secp->order);
														}
														writekey(true,&keyfound);
														notify_key_found(&keyfound);
													}
												}
											}
										}
									}

									if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH)	{
										if(FLAGENDOMORPHISM)	{
											for(l = 6;l < 12; l++)	{	//We check the array from 6 to 12(excluded) because we save the uncompressed information there
												r = address_check(publickeyhashrmd160_endomorphism[l][k],MAXLENGTHADDRESS);	//Check in Bloom filter
												if(r) {
													r = searchbinary(addressTable,publickeyhashrmd160_endomorphism[l][k],N);		//Check in Array using Binary search
													if(r) {
														keyfound.SetInt32(k);
														keyfound.Mult(&stride);
														keyfound.Add(&key_mpz);
														switch(l)	{
															case 6:
															case 7:
																publickey = secp->ComputePublicKey(&keyfound);
																secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
																if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
															break;
															case 8:
															case 9:
																keyfound.ModMulK1order(&lambda);
																publickey = secp->ComputePublicKey(&keyfound);
																secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
																if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
															break;
															case 10:
															case 11:
																keyfound.ModMulK1order(&lambda2);
																publickey = secp->ComputePublicKey(&keyfound);
																secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
																if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																	keyfound.Neg();
																	keyfound.Add(&secp->order);
																}
															break;
														}
														writekey(false,&keyfound);
														notify_key_found(&keyfound);
													}
												}
											}
										}
										else	{
											r = address_check(publickeyhashrmd160_uncompress[k],MAXLENGTHADDRESS);
											if(r) {
												r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
												if(r) {
													keyfound.SetInt32(k);
													keyfound.Mult(&stride);
													keyfound.Add(&key_mpz);
													writekey(false,&keyfound);
													notify_key_found(&keyfound);
												}
											}
										}
									}
								}
							}
							else if( FLAGCRYPTO == CRYPTO_ETH || FLAGCRYPTO == CRYPTO_ETC || FLAGCRYPTO == CRYPTO_ALL) {
								if(FLAGENDOMORPHISM)	{
									for(k = 0; k < 4;k++)	{
										for(l = 0;l < 6; l++)	{
											r = address_check(publickeyhashrmd160_endomorphism[l][k],MAXLENGTHADDRESS);
											if(r) {
												r = searchbinary(addressTable,publickeyhashrmd160_endomorphism[l][k],N);
												if(r) {												
													keyfound.SetInt32(k);
													keyfound.Mult(&stride);
													keyfound.Add(&key_mpz);
													switch(l)	{
														case 0:
														case 1:
															publickey = secp->ComputePublicKey(&keyfound);
															generate_binaddress_eth(publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
															if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																keyfound.Neg();
																keyfound.Add(&secp->order);
															}
														break;
														case 2:
														case 3:
															keyfound.ModMulK1order(&lambda);
															publickey = secp->ComputePublicKey(&keyfound);
															generate_binaddress_eth(publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
															if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																keyfound.Neg();
																keyfound.Add(&secp->order);
															}
														break;
														case 4:
														case 5:
															keyfound.ModMulK1order(&lambda2);
															publickey = secp->ComputePublicKey(&keyfound);
															generate_binaddress_eth(publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
															if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
																keyfound.Neg();
																keyfound.Add(&secp->order);
															}
														break;
													}
													writekeyeth(&keyfound);											
												}
											}
										}
									}
								}
								else	{
									for(k = 0; k < 4;k++)	{
										r = address_check(publickeyhashrmd160_uncompress[k],MAXLENGTHADDRESS);
										if(r) {
											r = searchbinary(addressTable,publickeyhashrmd160_uncompress[k],N);
											if(r) {
												keyfound.SetInt32(k);
												keyfound.Mult(&stride);
												keyfound.Add(&key_mpz);
												writekeyeth(&keyfound);
											}
										}
									}
								}
							}
						break;
						case MODE_XPOINT:
							for(k = 0; k < 4;k++)	{
								if(FLAGENDOMORPHISM)	{
									pts[(4*j)+k].x.Get32Bytes((unsigned char *)rawvalue);
									r = address_check(rawvalue,MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,rawvalue,N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											
											writekey(false,&keyfound);
											notify_key_found(&keyfound);
										}
									}
									endomorphism_beta[(j*4)+k].x.Get32Bytes((unsigned char *)rawvalue);
									r = address_check(rawvalue,MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,rawvalue,N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											keyfound.ModMulK1order(&lambda);
											
											writekey(false,&keyfound);
											notify_key_found(&keyfound);
										}
									}
									
									endomorphism_beta2[(j*4)+k].x.Get32Bytes((unsigned char *)rawvalue);
									r = address_check(rawvalue,MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,rawvalue,N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											keyfound.ModMulK1order(&lambda2);
											writekey(false,&keyfound);
											notify_key_found(&keyfound);
										}
									}
								}
								else	{
									pts[(4*j)+k].x.Get32Bytes((unsigned char *)rawvalue);
									r = address_check(rawvalue,MAXLENGTHADDRESS);
									if(r) {
										r = searchbinary(addressTable,rawvalue,N);
										if(r) {
											keyfound.SetInt32(k);
											keyfound.Mult(&stride);
											keyfound.Add(&key_mpz);
											
											writekey(false,&keyfound);
											notify_key_found(&keyfound);
										}
									}
								}
							}
						break;
					}
					count+=4;
					temp_stride.SetInt32(4);
					temp_stride.Mult(&stride);
					key_mpz.Add(&temp_stride);
				}
				/*
				if(FLAGDEBUG) {
					printf("\n[D] thread_process %i\n",__LINE__ -1 );
					fflush(stdout);
				}
				*/

				steps[thread_number]++;

				// Next start point (startP + GRP_SIZE*G)
				pp = startP;
				dy.ModSub(&_2Gn.y,&pp.y);

				_s.ModMulK1(&dy,&dx[i + 1]);
				_p.ModSquareK1(&_s);

				pp.x.ModNeg();
				pp.x.ModAdd(&_p);
				pp.x.ModSub(&_2Gn.x);

				//The Y value for the next start point always need to be calculated
				pp.y.ModSub(&_2Gn.x,&pp.x);
				pp.y.ModMulK1(&_s);
				pp.y.ModSub(&_2Gn.y);
				startP = pp;
			}while(count < N_SEQUENTIAL_MAX && continue_flag);
		}
	} while(continue_flag);
	ends[thread_number] = 1;
	return NULL;
}


#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_vanity(LPVOID vargp) {
#else
void *thread_process_vanity(void *vargp)	{
#endif
	struct tothread *tt;
	Point pts[CPU_GRP_SIZE];
	Point endomorphism_beta[CPU_GRP_SIZE];
	Point endomorphism_beta2[CPU_GRP_SIZE];
	Point endomorphism_negeted_point[4];
		
	Int dx[CPU_GRP_SIZE / 2 + 1];
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Point pp;	//point positive
	Point pn;	//point negative
	int l,pp_offset,pn_offset,i,hLength = (CPU_GRP_SIZE / 2 - 1);
	uint64_t j,count;
	Point R,temporal,publickey;
	int thread_number,continue_flag = 1,k;
	char *hextemp = NULL;
	char publickeyhashrmd160[20];
	char publickeyhashrmd160_uncompress[4][20];
	
	char publickeyhashrmd160_endomorphism[12][4][20];
	
	Int key_mpz,temp_stride,keyfound;
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	grp->Set(dx);
	
	
	//if FLAGENDOMORPHISM  == 1 and only compress search is enabled then there is no need to calculate the Y value value					
	
	bool calculate_y = FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH;
	
	/*
	if(FLAGDEBUG && thread_number == 0)	{
		printf("[D] vanity_rmd_targets = %i          fillllll\n",vanity_rmd_targets);
		printf("[D] vanity_rmd_total = %i\n",vanity_rmd_total);
		for(i =0; i < vanity_rmd_targets;i++)	{
			printf("[D] vanity_rmd_limits[%li] = %i\n",i,vanity_rmd_limits[i]);
			
		}
		printf("[D] vanity_rmd_minimun_bytes_check_length = %i\n",vanity_rmd_minimun_bytes_check_length);
	}
	*/
	

	do {
		if(FLAGSEARCHMODE == SEARCHMODE_SEQUENTIAL) {
			if(n_range_start.IsLower(&n_range_end))	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(write_random, INFINITE);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SEQUENTIAL_MAX);
				ReleaseMutex(write_random);
#else
				pthread_mutex_lock(&write_random);
				key_mpz.Set(&n_range_start);
				n_range_start.Add(N_SEQUENTIAL_MAX);
				pthread_mutex_unlock(&write_random);
#endif
			}
			else	{
				continue_flag = 0;
			}
		}
		else	{
#if defined(_WIN64) && !defined(__CYGWIN__)
			WaitForSingleObject(write_random, INFINITE);
			get_next_search_key(&key_mpz, &n_range_start, &n_range_end);
			ReleaseMutex(write_random);
#else
			pthread_mutex_lock(&write_random);
			get_next_search_key(&key_mpz, &n_range_start, &n_range_end);
			pthread_mutex_unlock(&write_random);
#endif
		}
		if(continue_flag)	{
			count = 0;
			if(FLAGMATRIX)	{
					hextemp = key_mpz.GetBase16();
					printf("Base key: %s thread %i\n",hextemp,thread_number);
					fflush(stdout);
					free(hextemp);
			}
			else	{
				if(FLAGQUIET == 0)	{
					hextemp = key_mpz.GetBase16();
					printf("\rBase key: %s     \r",hextemp);
					fflush(stdout);
					free(hextemp);
					THREADOUTPUT = 1;
				}
			}
			do {
				temp_stride.SetInt32(CPU_GRP_SIZE / 2);
				temp_stride.Mult(&stride);
				key_mpz.Add(&temp_stride);
	 			startP = secp->ComputePublicKey(&key_mpz);
				key_mpz.Sub(&temp_stride);

				for(i = 0; i < hLength; i++) {
					dx[i].ModSub(&Gn[i].x,&startP.x);
				}
			
				dx[i].ModSub(&Gn[i].x,&startP.x);  // For the first point
				dx[i + 1].ModSub(&_2Gn.x,&startP.x); // For the next center point
				grp->ModInv();

				pts[CPU_GRP_SIZE / 2] = startP;

				for(i = 0; i<hLength; i++) {
					pp = startP;
					pn = startP;

					// P = startP + i*G
					dy.ModSub(&Gn[i].y,&pp.y);

					_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;
					
					if(calculate_y)	{
						pp.y.ModSub(&Gn[i].x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
					}

					// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
					dyn.Set(&Gn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
					_p.ModSquareK1(&_s);            // _p = pow2(s)
					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

					if( calculate_y  )	{
						pn.y.ModSub(&Gn[i].x,&pn.x);
						pn.y.ModMulK1(&_s);
						pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
					}
					pp_offset = CPU_GRP_SIZE / 2 + (i + 1);
					pn_offset = CPU_GRP_SIZE / 2 - (i + 1);

					pts[pp_offset] = pp;
					pts[pn_offset] = pn;
					
					if(FLAGENDOMORPHISM)	{
						/*
							Q = (x,y)
							For any point Q
							Q*lambda = (x*beta mod p ,y)
							Q*lambda is a Scalar Multiplication
							x*beta is just a Multiplication (Very fast)
						*/
						
						if( calculate_y  )	{
							endomorphism_beta[pp_offset].y.Set(&pp.y);
							endomorphism_beta[pn_offset].y.Set(&pn.y);
							endomorphism_beta2[pp_offset].y.Set(&pp.y);
							endomorphism_beta2[pn_offset].y.Set(&pn.y);
						}
						endomorphism_beta[pp_offset].x.ModMulK1(&pp.x, &beta);
						endomorphism_beta[pn_offset].x.ModMulK1(&pn.x, &beta);
						endomorphism_beta2[pp_offset].x.ModMulK1(&pp.x, &beta2);
						endomorphism_beta2[pn_offset].x.ModMulK1(&pn.x, &beta2);
					}
				}
				/*
					Half point for endomorphism because pts[CPU_GRP_SIZE / 2] was not calcualte in the previous cycle
				*/
				if(FLAGENDOMORPHISM)	{
					if( calculate_y  )	{

						endomorphism_beta[CPU_GRP_SIZE / 2].y.Set(&pts[CPU_GRP_SIZE / 2].y);
						endomorphism_beta2[CPU_GRP_SIZE / 2].y.Set(&pts[CPU_GRP_SIZE / 2].y);
					}
					endomorphism_beta[CPU_GRP_SIZE / 2].x.ModMulK1(&pts[CPU_GRP_SIZE / 2].x, &beta);
					endomorphism_beta2[CPU_GRP_SIZE / 2].x.ModMulK1(&pts[CPU_GRP_SIZE / 2].x, &beta2);
				}
				
				// First point (startP - (GRP_SZIE/2)*G)
				pn = startP;
				dyn.Set(&Gn[i].y);
				dyn.ModNeg();
				dyn.ModSub(&pn.y);

				_s.ModMulK1(&dyn,&dx[i]);
				_p.ModSquareK1(&_s);

				pn.x.ModNeg();
				pn.x.ModAdd(&_p);
				pn.x.ModSub(&Gn[i].x);
				
				if(calculate_y )	{
					pn.y.ModSub(&Gn[i].x,&pn.x);
					pn.y.ModMulK1(&_s);
					pn.y.ModAdd(&Gn[i].y);
				}
				pts[0] = pn;
				
				/*
					First point for endomorphism because pts[0] was not calcualte previously
				*/
				if(FLAGENDOMORPHISM)	{
					if( calculate_y  )	{
						endomorphism_beta[0].y.Set(&pn.y);
						endomorphism_beta2[0].y.Set(&pn.y);
					}
					endomorphism_beta[0].x.ModMulK1(&pn.x, &beta);
					endomorphism_beta2[0].x.ModMulK1(&pn.x, &beta2);
				}
				
				for(j = 0; j < CPU_GRP_SIZE/4;j++)	{
					if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH ){
						if(FLAGENDOMORPHISM)	{
							secp->GetHash160_fromX(P2PKH,0x02,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[0][0],(uint8_t*)publickeyhashrmd160_endomorphism[0][1],(uint8_t*)publickeyhashrmd160_endomorphism[0][2],(uint8_t*)publickeyhashrmd160_endomorphism[0][3]);
							secp->GetHash160_fromX(P2PKH,0x03,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[1][0],(uint8_t*)publickeyhashrmd160_endomorphism[1][1],(uint8_t*)publickeyhashrmd160_endomorphism[1][2],(uint8_t*)publickeyhashrmd160_endomorphism[1][3]);

							secp->GetHash160_fromX(P2PKH,0x02,&endomorphism_beta[(j*4)].x,&endomorphism_beta[(j*4)+1].x,&endomorphism_beta[(j*4)+2].x,&endomorphism_beta[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[2][0],(uint8_t*)publickeyhashrmd160_endomorphism[2][1],(uint8_t*)publickeyhashrmd160_endomorphism[2][2],(uint8_t*)publickeyhashrmd160_endomorphism[2][3]);
							secp->GetHash160_fromX(P2PKH,0x03,&endomorphism_beta[(j*4)].x,&endomorphism_beta[(j*4)+1].x,&endomorphism_beta[(j*4)+2].x,&endomorphism_beta[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[3][0],(uint8_t*)publickeyhashrmd160_endomorphism[3][1],(uint8_t*)publickeyhashrmd160_endomorphism[3][2],(uint8_t*)publickeyhashrmd160_endomorphism[3][3]);

							secp->GetHash160_fromX(P2PKH,0x02,&endomorphism_beta2[(j*4)].x,&endomorphism_beta2[(j*4)+1].x,&endomorphism_beta2[(j*4)+2].x,&endomorphism_beta2[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[4][0],(uint8_t*)publickeyhashrmd160_endomorphism[4][1],(uint8_t*)publickeyhashrmd160_endomorphism[4][2],(uint8_t*)publickeyhashrmd160_endomorphism[4][3]);
							secp->GetHash160_fromX(P2PKH,0x03,&endomorphism_beta2[(j*4)].x,&endomorphism_beta2[(j*4)+1].x,&endomorphism_beta2[(j*4)+2].x,&endomorphism_beta2[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[5][0],(uint8_t*)publickeyhashrmd160_endomorphism[5][1],(uint8_t*)publickeyhashrmd160_endomorphism[5][2],(uint8_t*)publickeyhashrmd160_endomorphism[5][3]);

						}
						else	{
							secp->GetHash160_fromX(P2PKH,0x02,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[0][0],(uint8_t*)publickeyhashrmd160_endomorphism[0][1],(uint8_t*)publickeyhashrmd160_endomorphism[0][2],(uint8_t*)publickeyhashrmd160_endomorphism[0][3]);
							secp->GetHash160_fromX(P2PKH,0x03,&pts[(j*4)].x,&pts[(j*4)+1].x,&pts[(j*4)+2].x,&pts[(j*4)+3].x,(uint8_t*)publickeyhashrmd160_endomorphism[1][0],(uint8_t*)publickeyhashrmd160_endomorphism[1][1],(uint8_t*)publickeyhashrmd160_endomorphism[1][2],(uint8_t*)publickeyhashrmd160_endomorphism[1][3]);
						}
					}
					if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH)	{
						if(FLAGENDOMORPHISM)	{
							for(l = 0; l < 4; l++)	{
								endomorphism_negeted_point[l] = secp->Negation(pts[(j*4)+l]);
							}
							secp->GetHash160(P2PKH,false, pts[(j*4)], pts[(j*4)+1], pts[(j*4)+2], pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_endomorphism[6][0],(uint8_t*)publickeyhashrmd160_endomorphism[6][1],(uint8_t*)publickeyhashrmd160_endomorphism[6][2],(uint8_t*)publickeyhashrmd160_endomorphism[6][3]);
							secp->GetHash160(P2PKH,false,endomorphism_negeted_point[0] ,endomorphism_negeted_point[1],endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[7][0],(uint8_t*)publickeyhashrmd160_endomorphism[7][1],(uint8_t*)publickeyhashrmd160_endomorphism[7][2],(uint8_t*)publickeyhashrmd160_endomorphism[7][3]);
							for(l = 0; l < 4; l++)	{
								endomorphism_negeted_point[l] = secp->Negation(endomorphism_beta[(j*4)+l]);
							}
							secp->GetHash160(P2PKH,false,endomorphism_beta[(j*4)],  endomorphism_beta[(j*4)+1], endomorphism_beta[(j*4)+2], endomorphism_beta[(j*4)+3] ,(uint8_t*)publickeyhashrmd160_endomorphism[8][0],(uint8_t*)publickeyhashrmd160_endomorphism[8][1],(uint8_t*)publickeyhashrmd160_endomorphism[8][2],(uint8_t*)publickeyhashrmd160_endomorphism[8][3]);
							secp->GetHash160(P2PKH,false,endomorphism_negeted_point[0],endomorphism_negeted_point[1],endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[9][0],(uint8_t*)publickeyhashrmd160_endomorphism[9][1],(uint8_t*)publickeyhashrmd160_endomorphism[9][2],(uint8_t*)publickeyhashrmd160_endomorphism[9][3]);

							for(l = 0; l < 4; l++)	{
								endomorphism_negeted_point[l] = secp->Negation(endomorphism_beta2[(j*4)+l]);
							}
							secp->GetHash160(P2PKH,false, endomorphism_beta2[(j*4)],  endomorphism_beta2[(j*4)+1] ,  endomorphism_beta2[(j*4)+2] ,  endomorphism_beta2[(j*4)+3] ,(uint8_t*)publickeyhashrmd160_endomorphism[10][0],(uint8_t*)publickeyhashrmd160_endomorphism[10][1],(uint8_t*)publickeyhashrmd160_endomorphism[10][2],(uint8_t*)publickeyhashrmd160_endomorphism[10][3]);
							secp->GetHash160(P2PKH,false, endomorphism_negeted_point[0], endomorphism_negeted_point[1],   endomorphism_negeted_point[2],endomorphism_negeted_point[3],(uint8_t*)publickeyhashrmd160_endomorphism[11][0],(uint8_t*)publickeyhashrmd160_endomorphism[11][1],(uint8_t*)publickeyhashrmd160_endomorphism[11][2],(uint8_t*)publickeyhashrmd160_endomorphism[11][3]);
						}
						else	{
							secp->GetHash160(P2PKH,false,pts[(j*4)],pts[(j*4)+1],pts[(j*4)+2],pts[(j*4)+3],(uint8_t*)publickeyhashrmd160_uncompress[0],(uint8_t*)publickeyhashrmd160_uncompress[1],(uint8_t*)publickeyhashrmd160_uncompress[2],(uint8_t*)publickeyhashrmd160_uncompress[3]);
							
						}
					}
					for(k = 0; k < 4;k++)	{
						if(FLAGSEARCH == SEARCH_COMPRESS || FLAGSEARCH == SEARCH_BOTH ){
							if(FLAGENDOMORPHISM)	{
								for(l = 0;l < 6; l++)	{
									if(vanityrmdmatch((uint8_t*)publickeyhashrmd160_endomorphism[l][k]))	{
										// Here the given publickeyhashrmd160 match againts one of the vanity targets
										// We need to check which of the cases is it.

										keyfound.SetInt32(k);
										keyfound.Mult(&stride);
										keyfound.Add(&key_mpz);
										publickey = secp->ComputePublicKey(&keyfound);
										
										switch(l)	{
											case 0:	//Original point, prefix 02
												if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 02
											break;
											case 1:	//Original point, prefix 03
												if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 03
											break;
											case 2:	//Beta point, prefix 02
												keyfound.ModMulK1order(&lambda);
												if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 02
											break;
											case 3:	//Beta point, prefix 03											
												keyfound.ModMulK1order(&lambda);
												if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 02
											break;
											case 4:	//Beta^2 point, prefix 02
												keyfound.ModMulK1order(&lambda2);
												if(publickey.y.IsOdd())	{	//if the current publickey is odd that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 02
											break;
											case 5:	//Beta^2 point, prefix 03
												keyfound.ModMulK1order(&lambda2);
												if(publickey.y.IsEven())	{	//if the current publickey is even that means, we need to negate the keyfound to get the correct key
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
												// else we dont need to chage the current keyfound because it already have prefix 02
											break;
										}
										writevanitykey(true,&keyfound);
									}
								}
							}
							else	{
								for(l = 0;l < 2; l++)	{
									if(vanityrmdmatch((uint8_t*)publickeyhashrmd160_endomorphism[l][k]))	{
										keyfound.SetInt32(k);
										keyfound.Mult(&stride);
										keyfound.Add(&key_mpz);
										
										publickey = secp->ComputePublicKey(&keyfound);
										secp->GetHash160(P2PKH,true,publickey,(uint8_t*)publickeyhashrmd160);
										if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160,20) != 0){
											keyfound.Neg();
											keyfound.Add(&secp->order);
											//if(FLAGDEBUG) printf("[D] Key need to be negated\n");
										}
										writevanitykey(true,&keyfound);
									}
								}									
							}
						}
						if(FLAGSEARCH == SEARCH_UNCOMPRESS || FLAGSEARCH == SEARCH_BOTH)	{
							if(FLAGENDOMORPHISM)	{
								for(l = 6;l < 12; l++)	{
									if(vanityrmdmatch((uint8_t*)publickeyhashrmd160_endomorphism[l][k]))	{
										// Here the given publickeyhashrmd160 match againts one of the vanity targets
										// We need to check which of the cases is it.

										//rmd160toaddress_dst(publickeyhashrmd160_endomorphism[l][k],address);
										keyfound.SetInt32(k);
										keyfound.Mult(&stride);
										keyfound.Add(&key_mpz);
										
										
										switch(l)	{
											case 6:
											case 7:
												publickey = secp->ComputePublicKey(&keyfound);
												secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
												if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
											break;
											case 8:
											case 9:
												keyfound.ModMulK1order(&lambda);
												publickey = secp->ComputePublicKey(&keyfound);
												secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
												if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
											break;
											case 10:
											case 11:
												keyfound.ModMulK1order(&lambda2);
												publickey = secp->ComputePublicKey(&keyfound);
												secp->GetHash160(P2PKH,false,publickey,(uint8_t*)publickeyhashrmd160_uncompress[0]);
												if(memcmp(publickeyhashrmd160_endomorphism[l][k],publickeyhashrmd160_uncompress[0],20) != 0){
													keyfound.Neg();
													keyfound.Add(&secp->order);
												}
											break;
										}
										writevanitykey(false,&keyfound);
									}
								}

							}
							else	{
								if(vanityrmdmatch((uint8_t*)publickeyhashrmd160_uncompress[k]))	{
									keyfound.SetInt32(k);
									keyfound.Mult(&stride);
									keyfound.Add(&key_mpz);
									writevanitykey(false,&keyfound);
								}
							}
						}
						
					}

					count+=4;
					temp_stride.SetInt32(4);
					temp_stride.Mult(&stride);
					key_mpz.Add(&temp_stride);
				}
				steps[thread_number]++;

				// Next start point (startP + GRP_SIZE*G)
				pp = startP;
				dy.ModSub(&_2Gn.y,&pp.y);

				_s.ModMulK1(&dy,&dx[i + 1]);
				_p.ModSquareK1(&_s);

				pp.x.ModNeg();
				pp.x.ModAdd(&_p);
				pp.x.ModSub(&_2Gn.x);
				
				//The Y value for the next start point always need to be calculated
				pp.y.ModSub(&_2Gn.x,&pp.x);
				pp.y.ModMulK1(&_s);
				pp.y.ModSub(&_2Gn.y);
				startP = pp;
			}while(count < N_SEQUENTIAL_MAX && continue_flag);
		}
	} while(continue_flag);
	ends[thread_number] = 1;
	return NULL;
}

void _swap(struct address_value *a,struct address_value *b)	{
	struct address_value t;
	t  = *a;
	*a = *b;
	*b =  t;
}

void _sort(struct address_value *arr,int64_t n)	{
	uint32_t depthLimit = ((uint32_t) ceil(log(n))) * 2;
	_introsort(arr,depthLimit,n);
}

void _introsort(struct address_value *arr,uint32_t depthLimit, int64_t n) {
	int64_t p;
	if(n > 1)	{
		if(n <= 16) {
			_insertionsort(arr,n);
		}
		else	{
			if(depthLimit == 0) {
				_myheapsort(arr,n);
			}
			else	{
				p = _partition(arr,n);
				if(p > 0) _introsort(arr , depthLimit-1 , p);
				if(p < n) _introsort(&arr[p+1],depthLimit-1,n-(p+1));
			}
		}
	}
}

void _insertionsort(struct address_value *arr, int64_t n) {
	int64_t j;
	int64_t i;
	struct address_value key;
	for(i = 1; i < n ; i++ ) {
		key = arr[i];
		j= i-1;
		while(j >= 0 && memcmp(arr[j].value,key.value,20) > 0) {
			arr[j+1] = arr[j];
			j--;
		}
		arr[j+1] = key;
	}
}

int64_t _partition(struct address_value *arr, int64_t n)	{
	struct address_value pivot;
	int64_t r,left,right;
	r = n/2;
	pivot = arr[r];
	left = 0;
	right = n-1;
	do {
		while(left	< right && memcmp(arr[left].value,pivot.value,20) <= 0 )	{
			left++;
		}
		while(right >= left && memcmp(arr[right].value,pivot.value,20) > 0)	{
			right--;
		}
		if(left < right)	{
			if(left == r || right == r)	{
				if(left == r)	{
					r = right;
				}
				if(right == r)	{
					r = left;
				}
			}
			_swap(&arr[right],&arr[left]);
		}
	}while(left < right);
	if(right != r)	{
		_swap(&arr[right],&arr[r]);
	}
	return right;
}

void _heapify(struct address_value *arr, int64_t n, int64_t i) {
	int64_t largest = i;
	int64_t l = 2 * i + 1;
	int64_t r = 2 * i + 2;
	if (l < n && memcmp(arr[l].value,arr[largest].value,20) > 0)
		largest = l;
	if (r < n && memcmp(arr[r].value,arr[largest].value,20) > 0)
		largest = r;
	if (largest != i) {
		_swap(&arr[i],&arr[largest]);
		_heapify(arr, n, largest);
	}
}

void _myheapsort(struct address_value	*arr, int64_t n)	{
	int64_t i;
	for ( i = (n / 2) - 1; i >=	0; i--)	{
		_heapify(arr, n, i);
	}
	for ( i = n - 1; i > 0; i--) {
		_swap(&arr[0] , &arr[i]);
		_heapify(arr, i, 0);
	}
}

/*	OK	*/
void bsgs_swap(struct bsgs_xvalue *a,struct bsgs_xvalue *b)	{
	struct bsgs_xvalue t;
	t	= *a;
	*a = *b;
	*b =	t;
}

/*	OK	*/
void bsgs_sort(struct bsgs_xvalue *arr,int64_t n)	{
	uint32_t depthLimit = ((uint32_t) ceil(log(n))) * 2;
	bsgs_introsort(arr,depthLimit,n);
}

/*	OK	*/
void bsgs_introsort(struct bsgs_xvalue *arr,uint32_t depthLimit, int64_t n) {
	int64_t p;
	if(n > 1)	{
		if(n <= 16) {
			bsgs_insertionsort(arr,n);
		}
		else	{
			if(depthLimit == 0) {
				bsgs_myheapsort(arr,n);
			}
			else	{
				p = bsgs_partition(arr,n);
				if(p > 0) bsgs_introsort(arr , depthLimit-1 , p);
				if(p < n) bsgs_introsort(&arr[p+1],depthLimit-1,n-(p+1));
			}
		}
	}
}

/*	OK	*/
void bsgs_insertionsort(struct bsgs_xvalue *arr, int64_t n) {
	int64_t j;
	int64_t i;
	struct bsgs_xvalue key;
	for(i = 1; i < n ; i++ ) {
		key = arr[i];
		j= i-1;
		while(j >= 0 && memcmp(arr[j].value,key.value,BSGS_XVALUE_RAM) > 0) {
			arr[j+1] = arr[j];
			j--;
		}
		arr[j+1] = key;
	}
}

int64_t bsgs_partition(struct bsgs_xvalue *arr, int64_t n)	{
	struct bsgs_xvalue pivot;
	int64_t r,left,right;
	r = n/2;
	pivot = arr[r];
	left = 0;
	right = n-1;
	do {
		while(left	< right && memcmp(arr[left].value,pivot.value,BSGS_XVALUE_RAM) <= 0 )	{
			left++;
		}
		while(right >= left && memcmp(arr[right].value,pivot.value,BSGS_XVALUE_RAM) > 0)	{
			right--;
		}
		if(left < right)	{
			if(left == r || right == r)	{
				if(left == r)	{
					r = right;
				}
				if(right == r)	{
					r = left;
				}
			}
			bsgs_swap(&arr[right],&arr[left]);
		}
	}while(left < right);
	if(right != r)	{
		bsgs_swap(&arr[right],&arr[r]);
	}
	return right;
}

void bsgs_heapify(struct bsgs_xvalue *arr, int64_t n, int64_t i) {
	int64_t largest = i;
	int64_t l = 2 * i + 1;
	int64_t r = 2 * i + 2;
	if (l < n && memcmp(arr[l].value,arr[largest].value,BSGS_XVALUE_RAM) > 0)
		largest = l;
	if (r < n && memcmp(arr[r].value,arr[largest].value,BSGS_XVALUE_RAM) > 0)
		largest = r;
	if (largest != i) {
		bsgs_swap(&arr[i],&arr[largest]);
		bsgs_heapify(arr, n, largest);
	}
}

void bsgs_myheapsort(struct bsgs_xvalue	*arr, int64_t n)	{
	int64_t i;
	for ( i = (n / 2) - 1; i >=	0; i--)	{
		bsgs_heapify(arr, n, i);
	}
	for ( i = n - 1; i > 0; i--) {
		bsgs_swap(&arr[0] , &arr[i]);
		bsgs_heapify(arr, i, 0);
	}
}

int bsgs_searchbinary(struct bsgs_xvalue *buffer,char *data,int64_t array_length,uint64_t *r_value) {
	int64_t min,max,half,current;
	int r = 0,rcmp;
	min = 0;
	current = 0;
	max = array_length;
	half = array_length;
	while(!r && half >= 1) {
		half = (max - min)/2;
		rcmp = memcmp(data+16,buffer[current+half].value,BSGS_XVALUE_RAM);
		if(rcmp == 0)	{
			*r_value = buffer[current+half].index;
			r = 1;
		}
		else	{
			if(rcmp < 0) {
				max = (max-half);
			}
			else	{
				min = (min+half);
			}
			current = min;
		}
	}
	return r;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs(LPVOID vargp) {
#else
void *thread_process_bsgs(void *vargp)	{
#endif
	// File-related variables
	FILE* filekey;
	struct tothread* tt;

	// Character variables
	char xpoint_raw[32], *aux_c, *hextemp;

	// Integer variables
	Int base_key, keyfound;
	IntGroup* grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Int dy, dyn, _s, _p, km, intaux;

	// Point variables
	Point base_point, point_aux, point_found;
	Point startP;
	Point pp, pn;
	Point pts[CPU_GRP_SIZE];

	// Unsigned integer variables
	uint32_t k, l, r, salir, thread_number, cycles;

	// Other variables
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	grp->Set(dx);

	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}

	intaux.Set(&BSGS_M_double);
	intaux.Mult(CPU_GRP_SIZE/2);
	intaux.Add(&BSGS_M);
	
	do	{
	/*
		We do this in an atomic pthread_mutex operation to not affect others threads
		so BSGS_CURRENT is never the same between threads
	*/
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		if(FLAGSEARCHMODE != SEARCHMODE_SEQUENTIAL) {
			get_next_search_key(&base_key, &n_range_start, &n_range_end);
		}
		else {
			base_key.Set(&BSGS_CURRENT);
			BSGS_CURRENT.Add(&BSGS_N_double);
		}

#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif

		if(base_key.IsGreaterOrEqual(&n_range_end))
			break;
		
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		base_point = secp->ComputePublicKey(&base_key);
		km.Set(&base_key);
		km.Neg();
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
				uint32_t j = 0;
				while( j < cycles && bsgs_found[k]== 0 )	{
					int i;
					for(i = 0; i < hLength; i++) {
						dx[i].ModSub(&GSn[i].x,&startP.x);
					}
					dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
					dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point
					// Grouped ModInv
					grp->ModInv();
					/*
					We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
					We compute key in the positive and negative way from the center of the group
					*/
					// center point
					pts[CPU_GRP_SIZE / 2] = startP;
					for(i = 0; i<hLength; i++) {
						pp = startP;
						pn = startP;

						// P = startP + i*G
						dy.ModSub(&GSn[i].y,&pp.y);

						_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
#if 0
  pp.y.ModSub(&GSn[i].x,&pp.x);
  pp.y.ModMulK1(&_s);
  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif
						// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
  pn.y.ModSub(&GSn[i].x,&pn.x);
  pn.y.ModMulK1(&_s);
  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif

						pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
						pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
					}
					// First point (startP - (GRP_SZIE/2)*G)
					pn = startP;
					dyn.Set(&GSn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);
					_p.ModSquareK1(&_s);

					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&GSn[i].x);

#if 0
pn.y.ModSub(&GSn[i].x,&pn.x);
pn.y.ModMulK1(&_s);
pn.y.ModAdd(&GSn[i].y);
#endif
					pts[0] = pn;
					for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
						pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
						r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
						if(r) {
							r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
							if(r)	{
								hextemp = keyfound.GetBase16();
								printf("[+] Thread Key found privkey %s   \n",hextemp);
								point_found = secp->ComputePublicKey(&keyfound);
								aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
								printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif

								filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
								if(filekey != NULL)	{
									fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
									fclose(filekey);
								}
								free(hextemp);
								free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
				ReleaseMutex(write_keys);
#else
				pthread_mutex_unlock(&write_keys);
#endif
								bsgs_found[k] = 1;
								notify_key_found(&keyfound);
								notify_key_found(&keyfound);
								salir = 1;
								for(l = 0; l < bsgs_point_number && salir; l++)	{
									salir &= bsgs_found[l];
								}
								if(salir)	{
									printf("All points were found\n");
									exit(EXIT_FAILURE);
								}
							} //End if second check
						}//End if first check
					}// For for pts variable
					// Next start point (startP += (bsSize*GRP_SIZE).G)
					pp = startP;
					dy.ModSub(&_2GSn.y,&pp.y);

					_s.ModMulK1(&dy,&dx[i + 1]);
					_p.ModSquareK1(&_s);

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&_2GSn.x);

					pp.y.ModSub(&_2GSn.x,&pp.x);
					pp.y.ModMulK1(&_s);
					pp.y.ModSub(&_2GSn.y);
					startP = pp;
					
					j++;
				} // end while
			}// End if 
		}
		steps[thread_number]+=2;
	}while(1);
	ends[thread_number] = 1;
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_random(LPVOID vargp) {
#else
void *thread_process_bsgs_random(void *vargp)	{
#endif

	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound,n_range_random;
	Point base_point,point_aux,point_found;
	uint32_t l,k,r,salir,thread_number,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);


	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	intaux.Set(&BSGS_M_double);
	intaux.Mult(CPU_GRP_SIZE/2);
	intaux.Add(&BSGS_M);

	do	{
		
	
	/*          | Start Range	| End Range     |
		None	| 1             | EC.N          |
		-b	bit | Min bit value | Max bit value |
		-r	A:B | A             | B             |
	*/
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif

		get_next_search_key(&base_key, &n_range_start, &n_range_end);
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif

		if(FLAGMATRIX)	{
				aux_c = base_key.GetBase16();
				printf("[+] Thread 0x%s  \n",aux_c);
				fflush(stdout);
				free(aux_c);
		}
		else{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s  \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);


		/* We need to test individually every point in BSGS_Q */
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{			
				startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
				uint32_t j = 0;
				while( j < cycles && bsgs_found[k]== 0 )	{
				
					int i;
					for(i = 0; i < hLength; i++) {
						dx[i].ModSub(&GSn[i].x,&startP.x);
					}
					dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
					dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

					// Grouped ModInv
					grp->ModInv();
					
					/*
					We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
					We compute key in the positive and negative way from the center of the group
					*/

					// center point
					pts[CPU_GRP_SIZE / 2] = startP;
					
					for(i = 0; i<hLength; i++) {

						pp = startP;
						pn = startP;

						// P = startP + i*G
						dy.ModSub(&GSn[i].y,&pp.y);

						_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
						
#if 0
  pp.y.ModSub(&GSn[i].x,&pp.x);
  pp.y.ModMulK1(&_s);
  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

						// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
  pn.y.ModSub(&GSn[i].x,&pn.x);
  pn.y.ModMulK1(&_s);
  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


						pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
						pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

					}

					// First point (startP - (GRP_SZIE/2)*G)
					pn = startP;
					dyn.Set(&GSn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);
					_p.ModSquareK1(&_s);

					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&GSn[i].x);

#if 0
pn.y.ModSub(&GSn[i].x,&pn.x);
pn.y.ModMulK1(&_s);
pn.y.ModAdd(&GSn[i].y);
#endif

					pts[0] = pn;
					
					for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
						pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
						r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
						if(r) {
							r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
							if(r)	{
								hextemp = keyfound.GetBase16();
								printf("[+] Thread Key found privkey %s    \n",hextemp);
								point_found = secp->ComputePublicKey(&keyfound);
								aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
								printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif

								filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
								if(filekey != NULL)	{
									fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
									fclose(filekey);
								}
								free(hextemp);
								free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif

								bsgs_found[k] = 1;
								notify_key_found(&keyfound);
								salir = 1;
								for(l = 0; l < bsgs_point_number && salir; l++)	{
									salir &= bsgs_found[l];
								}
								if(salir)	{
									printf("All points were found\n");
									exit(EXIT_FAILURE);
								}
							} //End if second check
						}//End if first check
						
					}// For for pts variable
					
					// Next start point (startP += (bsSize*GRP_SIZE).G)
					
					pp = startP;
					dy.ModSub(&_2GSn.y,&pp.y);

					_s.ModMulK1(&dy,&dx[i + 1]);
					_p.ModSquareK1(&_s);

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&_2GSn.x);

					pp.y.ModSub(&_2GSn.x,&pp.x);
					pp.y.ModMulK1(&_s);
					pp.y.ModSub(&_2GSn.y);
					startP = pp;
					
					j++;
					
				}	//End While
			}	//End if
		} // End for with k bsgs_point_number

		steps[thread_number]+=2;
	}while(1);
	ends[thread_number] = 1;
	return NULL;
}


/*
	The bsgs_secondcheck function is made to perform a second BSGS search in a Range of less size.
	This funtion is made with the especific purpouse to USE a smaller bPtable in RAM.
*/
int bsgs_secondcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey)	{
	int i = 0,found = 0,r = 0;
	Int base_key;
	Point base_point,point_aux;
	Point BSGS_Q, BSGS_S,BSGS_Q_AMP;
	char xpoint_raw[32];


	base_key.Set(&BSGS_M_double);
	base_key.Mult((uint64_t) a);
	base_key.Add(start_range);

	base_point = secp->ComputePublicKey(&base_key);
	point_aux = secp->Negation(base_point);

	/*
		BSGS_S = Q - base_key
				 Q is the target Key
		base_key is the Start range + a*BSGS_M
	*/
	BSGS_S = secp->AddDirect(OriginalPointsBSGS[k_index],point_aux);
	BSGS_Q.Set(BSGS_S);
	do {
		BSGS_Q_AMP = secp->AddDirect(BSGS_Q,BSGS_AMP2[i]);
		BSGS_S.Set(BSGS_Q_AMP);
		BSGS_S.x.Get32Bytes((unsigned char *) xpoint_raw);
		r = bloom_check(&bloom_bPx2nd[(uint8_t) xpoint_raw[0]],xpoint_raw,32);
		if(r)	{
			found = bsgs_thirdcheck(&base_key,i,k_index,privatekey);
		}
		i++;
	}while(i < 32 && !found);
	return found;
}

int bsgs_thirdcheck(Int *start_range,uint32_t a,uint32_t k_index,Int *privatekey)	{
	uint64_t j = 0;
	int i = 0,found = 0,r = 0;
	Int base_key,calculatedkey;
	Point base_point,point_aux;
	Point BSGS_Q, BSGS_S,BSGS_Q_AMP;
	char xpoint_raw[32];

	base_key.SetInt32(a);
	base_key.Mult(&BSGS_M2_double);
	base_key.Add(start_range);

	base_point = secp->ComputePublicKey(&base_key);
	point_aux = secp->Negation(base_point);
	
	BSGS_S = secp->AddDirect(OriginalPointsBSGS[k_index],point_aux);
	BSGS_Q.Set(BSGS_S);
	
	do {
		BSGS_Q_AMP = secp->AddDirect(BSGS_Q,BSGS_AMP3[i]);
		BSGS_S.Set(BSGS_Q_AMP);
		BSGS_S.x.Get32Bytes((unsigned char *)xpoint_raw);
		r = bloom_check(&bloom_bPx3rd[(uint8_t)xpoint_raw[0]],xpoint_raw,32);
		if(r)	{
			r = bsgs_searchbinary(bPtable,xpoint_raw,bsgs_m3,&j);
			if(r)	{
				calcualteindex(i,&calculatedkey);
				privatekey->Set(&calculatedkey);
				privatekey->Add((uint64_t)(j+1));
				privatekey->Add(&base_key);
				point_aux = secp->ComputePublicKey(privatekey);
				if(point_aux.x.IsEqual(&OriginalPointsBSGS[k_index].x))	{
					found = 1;
				}
				else	{
					calcualteindex(i,&calculatedkey);
					privatekey->Set(&calculatedkey);
					privatekey->Sub((uint64_t)(j+1));
					privatekey->Add(&base_key);
					point_aux = secp->ComputePublicKey(privatekey);
					if(point_aux.x.IsEqual(&OriginalPointsBSGS[k_index].x))	{
						found = 1;
					}
				}
			}
		}
		else	{
			/*
				For some reason the AddDirect don't return 000000... value when the publickeys are the negated values from each other
				Why JLP?
				This is is an special case
			*/
			if(BSGS_Q.x.IsEqual(&BSGS_AMP3[i].x))	{
				calcualteindex(i,&calculatedkey);
				privatekey->Set(&calculatedkey);
				privatekey->Add(&base_key);
				found = 1;
			}
		}
		i++;
	}while(i < 32 && !found);
	return found;
}

void sleep_ms(int milliseconds)	{ // cross-platform sleep function
#if defined(_WIN64) && !defined(__CYGWIN__)
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    if (milliseconds >= 1000)
      sleep(milliseconds / 1000);
    usleep((milliseconds % 1000) * 1000);
#endif
}


void init_generator()	{
	Point G = secp->ComputePublicKey(&stride);
	Point g;
	g.Set(G);
	Gn.resize(CPU_GRP_SIZE / 2);
	Gn[0] = g;
	g = secp->DoubleDirect(g);
	Gn[1] = g;
	for(int i = 2; i < CPU_GRP_SIZE / 2; i++) {
		g = secp->AddDirect(g,G);
		Gn[i] = g;
	}
	_2Gn = secp->DoubleDirect(Gn[CPU_GRP_SIZE / 2 - 1]);
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_bPload(LPVOID vargp) {
#else
void *thread_bPload(void *vargp)	{
#endif

	char rawvalue[32];
	struct bPload *tt;
	uint64_t i_counter,j,nbStep,to;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];
	Int dy,dyn,_s,_p;
	Point pp,pn;
	
	int i,bloom_bP_index,hLength = (CPU_GRP_SIZE / 2 - 1) ,threadid;
	tt = (struct bPload *)vargp;
	Int km((uint64_t)(tt->from + 1));
	threadid = tt->threadid;
	//if(FLAGDEBUG) printf("[D] thread %i from %" PRIu64 " to %" PRIu64 "\n",threadid,tt->from,tt->to);
	
	i_counter = tt->from;

	nbStep = (tt->to - tt->from) / CPU_GRP_SIZE;
	
	if( ((tt->to - tt->from) % CPU_GRP_SIZE )  != 0)	{
		nbStep++;
	}
	//if(FLAGDEBUG) printf("[D] thread %i nbStep %" PRIu64 "\n",threadid,nbStep);
	to = tt->to;
	
	km.Add((uint64_t)(CPU_GRP_SIZE / 2));
	startP = secp->ComputePublicKey(&km);
	grp->Set(dx);
	for(uint64_t s=0;s<nbStep;s++) {
		for(i = 0; i < hLength; i++) {
			dx[i].ModSub(&Gn[i].x,&startP.x);
		}
		dx[i].ModSub(&Gn[i].x,&startP.x); // For the first point
		dx[i + 1].ModSub(&_2Gn.x,&startP.x);// For the next center point
		// Grouped ModInv
		grp->ModInv();

		// We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
		// We compute key in the positive and negative way from the center of the group
		// center point
		
		pts[CPU_GRP_SIZE / 2] = startP;	//Center point

		for(i = 0; i<hLength; i++) {
			pp = startP;
			pn = startP;

			// P = startP + i*G
			dy.ModSub(&Gn[i].y,&pp.y);

			_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pp.x.ModNeg();
			pp.x.ModAdd(&_p);
			pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

#if 0
			pp.y.ModSub(&Gn[i].x,&pp.x);
			pp.y.ModMulK1(&_s);
			pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
#endif

			// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
			dyn.Set(&Gn[i].y);
			dyn.ModNeg();
			dyn.ModSub(&pn.y);

			_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pn.x.ModNeg();
			pn.x.ModAdd(&_p);
			pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
			pn.y.ModSub(&Gn[i].x,&pn.x);
			pn.y.ModMulK1(&_s);
			pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
#endif

			pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
			pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
		}

		// First point (startP - (GRP_SZIE/2)*G)
		pn = startP;
		dyn.Set(&Gn[i].y);
		dyn.ModNeg();
		dyn.ModSub(&pn.y);

		_s.ModMulK1(&dyn,&dx[i]);
		_p.ModSquareK1(&_s);

		pn.x.ModNeg();
		pn.x.ModAdd(&_p);
		pn.x.ModSub(&Gn[i].x);

#if 0
		pn.y.ModSub(&Gn[i].x,&pn.x);
		pn.y.ModMulK1(&_s);
		pn.y.ModAdd(&Gn[i].y);
#endif

		pts[0] = pn;
		for(j=0;j<CPU_GRP_SIZE;j++)	{
			pts[j].x.Get32Bytes((unsigned char*)rawvalue);
			bloom_bP_index = (uint8_t)rawvalue[0];
			/*
			if(FLAGDEBUG){
				tohex_dst(rawvalue,32,hexraw);
				printf("%i : %s : %i\n",i_counter,hexraw,bloom_bP_index);
			}
			*/
			if(i_counter < bsgs_m3)	{
				if(!FLAGREADEDFILE3)	{
					memcpy(bPtable[i_counter].value,rawvalue+16,BSGS_XVALUE_RAM);
					bPtable[i_counter].index = i_counter;
				}
				if(!FLAGREADEDFILE4)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx3rd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx3rd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx3rd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx3rd_mutex[bloom_bP_index]);
#endif
				}
			}
			if(i_counter < bsgs_m2 && !FLAGREADEDFILE2)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(bloom_bPx2nd_mutex[bloom_bP_index], INFINITE);
				bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
				ReleaseMutex(bloom_bPx2nd_mutex[bloom_bP_index]);
#else
				pthread_mutex_lock(&bloom_bPx2nd_mutex[bloom_bP_index]);
				bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
				pthread_mutex_unlock(&bloom_bPx2nd_mutex[bloom_bP_index]);
#endif	
			}
			if(i_counter < to && !FLAGREADEDFILE1 )	{
#if defined(_WIN64) && !defined(__CYGWIN__)
				WaitForSingleObject(bloom_bP_mutex[bloom_bP_index], INFINITE);
				bloom_add(&bloom_bP[bloom_bP_index], rawvalue ,BSGS_BUFFERXPOINTLENGTH);
				ReleaseMutex(bloom_bP_mutex[bloom_bP_index]);
#else
				pthread_mutex_lock(&bloom_bP_mutex[bloom_bP_index]);
				bloom_add(&bloom_bP[bloom_bP_index], rawvalue ,BSGS_BUFFERXPOINTLENGTH);
				pthread_mutex_unlock(&bloom_bP_mutex[bloom_bP_index]);
#endif
			}
			i_counter++;
		}
		// Next start point (startP + GRP_SIZE*G)
		pp = startP;
		dy.ModSub(&_2Gn.y,&pp.y);

		_s.ModMulK1(&dy,&dx[i + 1]);
		_p.ModSquareK1(&_s);

		pp.x.ModNeg();
		pp.x.ModAdd(&_p);
		pp.x.ModSub(&_2Gn.x);

		pp.y.ModSub(&_2Gn.x,&pp.x);
		pp.y.ModMulK1(&_s);
		pp.y.ModSub(&_2Gn.y);
		startP = pp;
	}
	delete grp;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bPload_mutex[threadid], INFINITE);
	tt->finished = 1;
	ReleaseMutex(bPload_mutex[threadid]);
#else	
	pthread_mutex_lock(&bPload_mutex[threadid]);
	tt->finished = 1;
	pthread_mutex_unlock(&bPload_mutex[threadid]);
	pthread_exit(NULL);
#endif
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_bPload_2blooms(LPVOID vargp) {
#else
void *thread_bPload_2blooms(void *vargp)	{
#endif
	char rawvalue[32];
	struct bPload *tt;
	uint64_t i_counter,j,nbStep; //,to;
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];
	Int dy,dyn,_s,_p;
	Point pp,pn;
	int i,bloom_bP_index,hLength = (CPU_GRP_SIZE / 2 - 1) ,threadid;
	tt = (struct bPload *)vargp;
	Int km((uint64_t)(tt->from +1 ));
	threadid = tt->threadid;
	
	i_counter = tt->from;

	nbStep = (tt->to - (tt->from)) / CPU_GRP_SIZE;
	
	if( ((tt->to - (tt->from)) % CPU_GRP_SIZE )  != 0)	{
		nbStep++;
	}
	//if(FLAGDEBUG) printf("[D] thread %i nbStep %" PRIu64 "\n",threadid,nbStep);
	//to = tt->to;
	
	km.Add((uint64_t)(CPU_GRP_SIZE / 2));
	startP = secp->ComputePublicKey(&km);
	grp->Set(dx);
	for(uint64_t s=0;s<nbStep;s++) {
		for(i = 0; i < hLength; i++) {
			dx[i].ModSub(&Gn[i].x,&startP.x);
		}
		dx[i].ModSub(&Gn[i].x,&startP.x); // For the first point
		dx[i + 1].ModSub(&_2Gn.x,&startP.x);// For the next center point
		// Grouped ModInv
		grp->ModInv();

		// We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
		// We compute key in the positive and negative way from the center of the group
		// center point
		
		pts[CPU_GRP_SIZE / 2] = startP;	//Center point

		for(i = 0; i<hLength; i++) {
			pp = startP;
			pn = startP;

			// P = startP + i*G
			dy.ModSub(&Gn[i].y,&pp.y);

			_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pp.x.ModNeg();
			pp.x.ModAdd(&_p);
			pp.x.ModSub(&Gn[i].x);           // rx = pow2(s) - p1.x - p2.x;

#if 0
			pp.y.ModSub(&Gn[i].x,&pp.x);
			pp.y.ModMulK1(&_s);
			pp.y.ModSub(&Gn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);
#endif

			// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
			dyn.Set(&Gn[i].y);
			dyn.ModNeg();
			dyn.ModSub(&pn.y);

			_s.ModMulK1(&dyn,&dx[i]);      // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
			_p.ModSquareK1(&_s);            // _p = pow2(s)

			pn.x.ModNeg();
			pn.x.ModAdd(&_p);
			pn.x.ModSub(&Gn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
			pn.y.ModSub(&Gn[i].x,&pn.x);
			pn.y.ModMulK1(&_s);
			pn.y.ModAdd(&Gn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);
#endif

			pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
			pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;
		}

		// First point (startP - (GRP_SZIE/2)*G)
		pn = startP;
		dyn.Set(&Gn[i].y);
		dyn.ModNeg();
		dyn.ModSub(&pn.y);

		_s.ModMulK1(&dyn,&dx[i]);
		_p.ModSquareK1(&_s);

		pn.x.ModNeg();
		pn.x.ModAdd(&_p);
		pn.x.ModSub(&Gn[i].x);

#if 0
		pn.y.ModSub(&Gn[i].x,&pn.x);
		pn.y.ModMulK1(&_s);
		pn.y.ModAdd(&Gn[i].y);
#endif

		pts[0] = pn;
		for(j=0;j<CPU_GRP_SIZE;j++)	{
			pts[j].x.Get32Bytes((unsigned char*)rawvalue);
			bloom_bP_index = (uint8_t)rawvalue[0];
			if(i_counter < bsgs_m3)	{
				if(!FLAGREADEDFILE3)	{
					memcpy(bPtable[i_counter].value,rawvalue+16,BSGS_XVALUE_RAM);
					bPtable[i_counter].index = i_counter;
				}
				if(!FLAGREADEDFILE4)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx3rd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx3rd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx3rd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx3rd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx3rd_mutex[bloom_bP_index]);
#endif
				}
			}
			if(i_counter < bsgs_m2 && !FLAGREADEDFILE2)	{
#if defined(_WIN64) && !defined(__CYGWIN__)
					WaitForSingleObject(bloom_bPx2nd_mutex[bloom_bP_index], INFINITE);
					bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					ReleaseMutex(bloom_bPx2nd_mutex[bloom_bP_index]);
#else
					pthread_mutex_lock(&bloom_bPx2nd_mutex[bloom_bP_index]);
					bloom_add(&bloom_bPx2nd[bloom_bP_index], rawvalue, BSGS_BUFFERXPOINTLENGTH);
					pthread_mutex_unlock(&bloom_bPx2nd_mutex[bloom_bP_index]);
#endif			
			}
			i_counter++;
		}
		// Next start point (startP + GRP_SIZE*G)
		pp = startP;
		dy.ModSub(&_2Gn.y,&pp.y);

		_s.ModMulK1(&dy,&dx[i + 1]);
		_p.ModSquareK1(&_s);

		pp.x.ModNeg();
		pp.x.ModAdd(&_p);
		pp.x.ModSub(&_2Gn.x);

		pp.y.ModSub(&_2Gn.x,&pp.x);
		pp.y.ModMulK1(&_s);
		pp.y.ModSub(&_2Gn.y);
		startP = pp;
	}
	delete grp;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bPload_mutex[threadid], INFINITE);
	tt->finished = 1;
	ReleaseMutex(bPload_mutex[threadid]);
#else	
	pthread_mutex_lock(&bPload_mutex[threadid]);
	tt->finished = 1;
	pthread_mutex_unlock(&bPload_mutex[threadid]);
	pthread_exit(NULL);
#endif
	return NULL;
}

/* This function perform the KECCAK Opetation*/
void KECCAK_256(uint8_t *source, size_t size,uint8_t *dst)	{
	SHA3_256_CTX ctx;
	SHA3_256_Init(&ctx);
	SHA3_256_Update(&ctx,source,size);
	KECCAK_256_Final(dst,&ctx);
}

/* This function takes in two parameters:

publickey: a reference to a Point object representing a public key.
dst_address: a pointer to an unsigned char array where the generated binary address will be stored.
The function is designed to generate a binary address for Ethereum using the given public key.
It first extracts the x and y coordinates of the public key as 32-byte arrays, and concatenates them
to form a 64-byte array called bin_publickey. Then, it applies the KECCAK-256 hashing algorithm to
bin_publickey to generate the binary address, which is stored in dst_address. */

void generate_binaddress_eth(Point &publickey,unsigned char *dst_address)	{
	unsigned char bin_publickey[64];
	publickey.x.Get32Bytes(bin_publickey);
	publickey.y.Get32Bytes(bin_publickey+32);
	KECCAK_256(bin_publickey, 64, bin_publickey);
	memcpy(dst_address,bin_publickey+12,20);
}

void generate_binaddress_ltc(Point &publickey,unsigned char *dst_address)	{
	secp->GetHash160(P2PKH,true,publickey,dst_address);
}

void generate_binaddress_btg(Point &publickey,unsigned char *dst_address)	{
	secp->GetHash160(P2PKH,true,publickey,dst_address);
}

void rmd160toaddress_ltc(char *rmd,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = 0x30;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc ltc\n");
	}
}

void rmd160toaddress_btg(char *rmd,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = 0x26;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc btg\n");
	}
}

void rmd160toaddress_doge(char *rmd,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = 0x1e;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc doge\n");
	}
}

void rmd160toaddress_xrp(char *rmd,char *dst)	{
	char digest[60];
	size_t pubaddress_size = 40;
	digest[0] = 0x00;
	memcpy(digest+1,rmd,20);
	sha256((uint8_t*)digest, 21,(uint8_t*) digest+21);
	sha256((uint8_t*)digest+21, 32,(uint8_t*) digest+21);
	if(!b58enc(dst,&pubaddress_size,digest,25)){
		fprintf(stderr,"error b58enc xrp\n");
	}
}

int node_check_balance(const char *address, int crypto_type) {
	char cmd[2048];
	char result[4096];
	FILE *fp;

	if(NODE_RPC_URL && crypto_type == CRYPTO_BTC) {
		char rpc_user[256] = "";
		char rpc_pass[256] = "";
		char rpc_host[256] = "127.0.0.1";
		int rpc_port = 8332;

		sscanf(NODE_RPC_URL, "http://%[^:]:%[^@]@%[^:]:%d", rpc_user, rpc_pass, rpc_host, &rpc_port);

		char json_payload[512];
		snprintf(json_payload, sizeof(json_payload),
			"{\"jsonrpc\":\"1.0\",\"id\":\"keyhunt\",\"method\":\"scantxoutset\",\"params\":[[\"raw(%s)\"]]}", address);

		snprintf(cmd, sizeof(cmd),
			"curl -s -u %s:%s -H \"Content-Type: application/json\" -d '%s' http://%s:%d/ 2>/dev/null",
			rpc_user, rpc_pass, json_payload, rpc_host, rpc_port);
	}
	else {
		switch(crypto_type) {
			case CRYPTO_BTC:
				snprintf(cmd, sizeof(cmd), "curl -s \"https://blockstream.info/api/address/%s\" 2>/dev/null", address);
				break;
			case CRYPTO_ETH:
			case CRYPTO_ETC:
				snprintf(cmd, sizeof(cmd), "curl -s \"https://api.etherscan.io/api?module=account&action=balance&address=%s&tag=latest\" 2>/dev/null", address);
				break;
			case CRYPTO_LTC:
				snprintf(cmd, sizeof(cmd), "curl -s \"https://api.blockcypher.com/v1/ltc/main/addrs/%s/balance\" 2>/dev/null", address);
				break;
			default:
				return -1;
		}
	}

	fp = popen(cmd, "r");
	if(!fp) return -1;

	int bytes_read = fread(result, 1, sizeof(result) - 1, fp);
	result[bytes_read] = '\0';
	pclose(fp);

	if(bytes_read == 0) return -1;

	if(strstr(result, "\"final_balance\":0") || strstr(result, "\"balance\":\"0\"") || strstr(result, "\"amount\":\"0\"")) {
		return 0;
	}

	if(strstr(result, "\"final_balance\"") || strstr(result, "\"balance\"") || strstr(result, "\"amount\"")) {
		return 1;
	}

	return -1;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_dance(LPVOID vargp) {
#else
void *thread_process_bsgs_dance(void *vargp)	{
#endif

	Point pts[CPU_GRP_SIZE];
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pp,pn,startP,base_point,point_aux,point_found;
	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound,dy,dyn,_s,_p,km,intaux;
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	uint32_t k,l,r,salir,thread_number,entrar,cycles;
	int hLength = (CPU_GRP_SIZE / 2 - 1);	

	grp->Set(dx);
	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	intaux.Set(&BSGS_M_double);
	intaux.Mult(CPU_GRP_SIZE/2);
	intaux.Add(&BSGS_M);
	
	entrar = 1;
	
	
	/*
		while base_key is less than n_range_end then:
	*/
	do	{
		r = rand() % 3;
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(bsgs_thread, INFINITE);
#else
	pthread_mutex_lock(&bsgs_thread);
#endif
	if(FLAGSEARCHMODE != SEARCHMODE_SEQUENTIAL && FLAGSEARCHMODE != SEARCHMODE_RANDOM) {
		get_next_search_key(&base_key, &n_range_start, &n_range_end);
	}
	else {
	switch(r)	{
		case 0:	//TOP
			if(n_range_end.IsGreater(&BSGS_CURRENT))	{
				/*
					n_range_end.Sub(&BSGS_N);
					n_range_end.Sub(&BSGS_N);
				*/
					n_range_end.Sub(&BSGS_N_double);
					if(n_range_end.IsLower(&BSGS_CURRENT))	{
						base_key.Set(&BSGS_CURRENT);
					}
					else	{
						base_key.Set(&n_range_end);
					}
			}
			else	{
				entrar = 0;
			}
		break;
		case 1: //BOTTOM
			if(BSGS_CURRENT.IsLower(&n_range_end))	{
				base_key.Set(&BSGS_CURRENT);
				//BSGS_N_double
				BSGS_CURRENT.Add(&BSGS_N_double);
				/*
				BSGS_CURRENT.Add(&BSGS_N);
				BSGS_CURRENT.Add(&BSGS_N);
				*/
			}
			else	{
				entrar = 0;
			}
		break;
		case 2: //random - middle
			base_key.Rand(&BSGS_CURRENT,&n_range_end);
		break;
	} // end switch
	} // end else (sequential/random dance mode)
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(bsgs_thread);
#else
	pthread_mutex_unlock(&bsgs_thread);
#endif

		if(entrar == 0)
			break;
			
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
				uint32_t j = 0;
				while( j < cycles && bsgs_found[k]== 0 )	{
				
					int i;
					
					for(i = 0; i < hLength; i++) {
						dx[i].ModSub(&GSn[i].x,&startP.x);
					}
					dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
					dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

					// Grouped ModInv
					grp->ModInv();
					
					/*
					We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
					We compute key in the positive and negative way from the center of the group
					*/

					// center point
					pts[CPU_GRP_SIZE / 2] = startP;
					
					for(i = 0; i<hLength; i++) {

						pp = startP;
						pn = startP;

						// P = startP + i*G
						dy.ModSub(&GSn[i].y,&pp.y);

						_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
						
#if 0
  pp.y.ModSub(&GSn[i].x,&pp.x);
  pp.y.ModMulK1(&_s);
  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

						// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
  pn.y.ModSub(&GSn[i].x,&pn.x);
  pn.y.ModMulK1(&_s);
  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


						pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
						pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

					}

					// First point (startP - (GRP_SZIE/2)*G)
					pn = startP;
					dyn.Set(&GSn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);
					_p.ModSquareK1(&_s);

					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&GSn[i].x);

#if 0
pn.y.ModSub(&GSn[i].x,&pn.x);
pn.y.ModMulK1(&_s);
pn.y.ModAdd(&GSn[i].y);
#endif

					pts[0] = pn;
					
					for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
						pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
						r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
						if(r) {
							r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
							if(r)	{
								hextemp = keyfound.GetBase16();
								printf("[+] Thread Key found privkey %s   \n",hextemp);
								point_found = secp->ComputePublicKey(&keyfound);
								aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
								printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif

								filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
								if(filekey != NULL)	{
									fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
									fclose(filekey);
								}
								free(hextemp);
								free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif

								bsgs_found[k] = 1;
								notify_key_found(&keyfound);
								salir = 1;
								for(l = 0; l < bsgs_point_number && salir; l++)	{
									salir &= bsgs_found[l];
								}
								if(salir)	{
									printf("All points were found\n");
									exit(EXIT_FAILURE);
								}
							} //End if second check
						}//End if first check
						
					}// For for pts variable
					
					// Next start point (startP += (bsSize*GRP_SIZE).G)
					
					pp = startP;
					dy.ModSub(&_2GSn.y,&pp.y);

					_s.ModMulK1(&dy,&dx[i + 1]);
					_p.ModSquareK1(&_s);

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&_2GSn.x);

					pp.y.ModSub(&_2GSn.x,&pp.x);
					pp.y.ModMulK1(&_s);
					pp.y.ModSub(&_2GSn.y);
					startP = pp;
					
					j++;
				}//while all the aMP points
			}// End if 
		}
		steps[thread_number]+=2;
	}while(1);
	ends[thread_number] = 1;
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_backward(LPVOID vargp) {
#else
void *thread_process_bsgs_backward(void *vargp)	{
#endif
	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t k,l,r,salir,thread_number,entrar,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);

	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	
	intaux.Set(&BSGS_M_double);
	intaux.Mult(CPU_GRP_SIZE/2);
	intaux.Add(&BSGS_M);
	
	entrar = 1;
	/*
		while base_key is less than n_range_end then:
	*/
	do	{

#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif
		if(FLAGSEARCHMODE != SEARCHMODE_SEQUENTIAL) {
			get_next_search_key(&base_key, &n_range_start, &n_range_end);
		}
		else {
			if(n_range_end.IsGreater(&n_range_start))	{
				n_range_end.Sub(&BSGS_N_double);
				if(n_range_end.IsLower(&n_range_start))	{
					base_key.Set(&n_range_start);
				}
				else	{
					base_key.Set(&n_range_end);
				}
			}
			else	{
				entrar = 0;
			}
		}
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif
		if(entrar == 0)
			break;
		
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
				startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
				uint32_t j = 0;
				while( j < cycles && bsgs_found[k]== 0 )	{
					int i;
					for(i = 0; i < hLength; i++) {
						dx[i].ModSub(&GSn[i].x,&startP.x);
					}
					dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
					dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

					// Grouped ModInv
					grp->ModInv();
					
					/*
					We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
					We compute key in the positive and negative way from the center of the group
					*/

					// center point
					pts[CPU_GRP_SIZE / 2] = startP;
					
					for(i = 0; i<hLength; i++) {

						pp = startP;
						pn = startP;

						// P = startP + i*G
						dy.ModSub(&GSn[i].y,&pp.y);

						_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
						
#if 0
  pp.y.ModSub(&GSn[i].x,&pp.x);
  pp.y.ModMulK1(&_s);
  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

						// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
						_p.ModSquareK1(&_s);            // _p = pow2(s)

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
  pn.y.ModSub(&GSn[i].x,&pn.x);
  pn.y.ModMulK1(&_s);
  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


						pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
						pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

					}

					// First point (startP - (GRP_SZIE/2)*G)
					pn = startP;
					dyn.Set(&GSn[i].y);
					dyn.ModNeg();
					dyn.ModSub(&pn.y);

					_s.ModMulK1(&dyn,&dx[i]);
					_p.ModSquareK1(&_s);

					pn.x.ModNeg();
					pn.x.ModAdd(&_p);
					pn.x.ModSub(&GSn[i].x);

#if 0
pn.y.ModSub(&GSn[i].x,&pn.x);
pn.y.ModMulK1(&_s);
pn.y.ModAdd(&GSn[i].y);
#endif

					pts[0] = pn;
					
					for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
						pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
						r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
						if(r) {
							r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
							if(r)	{
								hextemp = keyfound.GetBase16();
								printf("[+] Thread Key found privkey %s   \n",hextemp);
								point_found = secp->ComputePublicKey(&keyfound);
								aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
								printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								WaitForSingleObject(write_keys, INFINITE);
#else
								pthread_mutex_lock(&write_keys);
#endif

								filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
								if(filekey != NULL)	{
									fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
									fclose(filekey);
								}
								free(hextemp);
								free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif

								bsgs_found[k] = 1;
								notify_key_found(&keyfound);
								salir = 1;
								for(l = 0; l < bsgs_point_number && salir; l++)	{
									salir &= bsgs_found[l];
								}
								if(salir)	{
									printf("All points were found\n");
									exit(EXIT_FAILURE);
								}
							} //End if second check
						}//End if first check
						
					}// For for pts variable
					
					// Next start point (startP += (bsSize*GRP_SIZE).G)
					
					pp = startP;
					dy.ModSub(&_2GSn.y,&pp.y);

					_s.ModMulK1(&dy,&dx[i + 1]);
					_p.ModSquareK1(&_s);

					pp.x.ModNeg();
					pp.x.ModAdd(&_p);
					pp.x.ModSub(&_2GSn.x);

					pp.y.ModSub(&_2GSn.x,&pp.x);
					pp.y.ModMulK1(&_s);
					pp.y.ModSub(&_2GSn.y);
					startP = pp;
					j++;
				}//while all the aMP points
			}// End if 
		}
		steps[thread_number]+=2;
	}while(1);
	ends[thread_number] = 1;
	return NULL;
}

#if defined(_WIN64) && !defined(__CYGWIN__)
DWORD WINAPI thread_process_bsgs_both(LPVOID vargp) {
#else
void *thread_process_bsgs_both(void *vargp)	{
#endif
	FILE *filekey;
	struct tothread *tt;
	char xpoint_raw[32],*aux_c,*hextemp;
	Int base_key,keyfound;
	Point base_point,point_aux,point_found;
	uint32_t k,l,r,salir,thread_number,entrar,cycles;
	
	IntGroup *grp = new IntGroup(CPU_GRP_SIZE / 2 + 1);
	Point startP;
	
	int hLength = (CPU_GRP_SIZE / 2 - 1);
	
	Int dx[CPU_GRP_SIZE / 2 + 1];
	Point pts[CPU_GRP_SIZE];

	Int dy;
	Int dyn;
	Int _s;
	Int _p;
	Int km,intaux;
	Point pp;
	Point pn;
	grp->Set(dx);

	
	tt = (struct tothread *)vargp;
	thread_number = tt->nt;
	free(tt);
	
	cycles = bsgs_aux / 1024;
	if(bsgs_aux % 1024 != 0)	{
		cycles++;
	}
	intaux.Set(&BSGS_M_double);
	intaux.Mult(CPU_GRP_SIZE/2);
	intaux.Add(&BSGS_M);
	
	entrar = 1;
	
	
	/*
		while BSGS_CURRENT is less than n_range_end 
	*/
	do	{

		r = rand() % 2;
#if defined(_WIN64) && !defined(__CYGWIN__)
		WaitForSingleObject(bsgs_thread, INFINITE);
#else
		pthread_mutex_lock(&bsgs_thread);
#endif
		if(FLAGSEARCHMODE != SEARCHMODE_SEQUENTIAL && FLAGSEARCHMODE != SEARCHMODE_RANDOM) {
			get_next_search_key(&base_key, &n_range_start, &n_range_end);
		}
		else {
		switch(r)	{
			case 0:	//TOP
				if(n_range_end.IsGreater(&BSGS_CURRENT))	{
						n_range_end.Sub(&BSGS_N_double);
						/*
						n_range_end.Sub(&BSGS_N);
						n_range_end.Sub(&BSGS_N);
						*/
						if(n_range_end.IsLower(&BSGS_CURRENT))	{
							base_key.Set(&BSGS_CURRENT);
						}
						else	{
							base_key.Set(&n_range_end);
						}
				}
				else	{
					entrar = 0;
				}
			break;
			case 1: //BOTTOM
				if(BSGS_CURRENT.IsLower(&n_range_end))	{
					base_key.Set(&BSGS_CURRENT);
					//BSGS_N_double
					BSGS_CURRENT.Add(&BSGS_N_double);
					/*
					BSGS_CURRENT.Add(&BSGS_N);
					BSGS_CURRENT.Add(&BSGS_N);
					*/
				}
				else	{
					entrar = 0;
				}
			break;
		}
		} // end else (sequential/random both mode)
#if defined(_WIN64) && !defined(__CYGWIN__)
		ReleaseMutex(bsgs_thread);
#else
		pthread_mutex_unlock(&bsgs_thread);
#endif

		if(entrar == 0)
			break;

		
		if(FLAGMATRIX)	{
			aux_c = base_key.GetBase16();
			printf("[+] Thread 0x%s \n",aux_c);
			fflush(stdout);
			free(aux_c);
		}
		else	{
			if(FLAGQUIET == 0){
				aux_c = base_key.GetBase16();
				printf("\r[+] Thread 0x%s   \r",aux_c);
				fflush(stdout);
				free(aux_c);
				THREADOUTPUT = 1;
			}
		}
		
		base_point = secp->ComputePublicKey(&base_key);

		km.Set(&base_key);
		km.Neg();
		
		km.Add(&secp->order);
		km.Sub(&intaux);
		point_aux = secp->ComputePublicKey(&km);
		
		for(k = 0; k < bsgs_point_number ; k++)	{
			if(bsgs_found[k] == 0)	{
					startP  = secp->AddDirect(OriginalPointsBSGS[k],point_aux);
					uint32_t j = 0;
					while( j < cycles && bsgs_found[k]== 0 )	{
						int i;
						for(i = 0; i < hLength; i++) {
							dx[i].ModSub(&GSn[i].x,&startP.x);
						}
						dx[i].ModSub(&GSn[i].x,&startP.x);  // For the first point
						dx[i+1].ModSub(&_2GSn.x,&startP.x); // For the next center point

						// Grouped ModInv
						grp->ModInv();
						
						/*
						We use the fact that P + i*G and P - i*G has the same deltax, so the same inverse
						We compute key in the positive and negative way from the center of the group
						*/

						// center point
						pts[CPU_GRP_SIZE / 2] = startP;
						
						for(i = 0; i<hLength; i++) {

							pp = startP;
							pn = startP;

							// P = startP + i*G
							dy.ModSub(&GSn[i].y,&pp.y);

							_s.ModMulK1(&dy,&dx[i]);        // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pp.x.ModNeg();
							pp.x.ModAdd(&_p);
							pp.x.ModSub(&GSn[i].x);           // rx = pow2(s) - p1.x - p2.x;
							
#if 0
	  pp.y.ModSub(&GSn[i].x,&pp.x);
	  pp.y.ModMulK1(&_s);
	  pp.y.ModSub(&GSn[i].y);           // ry = - p2.y - s*(ret.x-p2.x);  
#endif

							// P = startP - i*G  , if (x,y) = i*G then (x,-y) = -i*G
							dyn.Set(&GSn[i].y);
							dyn.ModNeg();
							dyn.ModSub(&pn.y);

							_s.ModMulK1(&dyn,&dx[i]);       // s = (p2.y-p1.y)*inverse(p2.x-p1.x);
							_p.ModSquareK1(&_s);            // _p = pow2(s)

							pn.x.ModNeg();
							pn.x.ModAdd(&_p);
							pn.x.ModSub(&GSn[i].x);          // rx = pow2(s) - p1.x - p2.x;

#if 0
	  pn.y.ModSub(&GSn[i].x,&pn.x);
	  pn.y.ModMulK1(&_s);
	  pn.y.ModAdd(&GSn[i].y);          // ry = - p2.y - s*(ret.x-p2.x);  
#endif


							pts[CPU_GRP_SIZE / 2 + (i + 1)] = pp;
							pts[CPU_GRP_SIZE / 2 - (i + 1)] = pn;

						}

						// First point (startP - (GRP_SZIE/2)*G)
						pn = startP;
						dyn.Set(&GSn[i].y);
						dyn.ModNeg();
						dyn.ModSub(&pn.y);

						_s.ModMulK1(&dyn,&dx[i]);
						_p.ModSquareK1(&_s);

						pn.x.ModNeg();
						pn.x.ModAdd(&_p);
						pn.x.ModSub(&GSn[i].x);

#if 0
	pn.y.ModSub(&GSn[i].x,&pn.x);
	pn.y.ModMulK1(&_s);
	pn.y.ModAdd(&GSn[i].y);
#endif

						pts[0] = pn;
						
						for(int i = 0; i<CPU_GRP_SIZE && bsgs_found[k]== 0; i++) {
							pts[i].x.Get32Bytes((unsigned char*)xpoint_raw);
							r = bloom_check(&bloom_bP[((unsigned char)xpoint_raw[0])],xpoint_raw,32);
							if(r) {
								r = bsgs_secondcheck(&base_key,((j*1024) + i),k,&keyfound);
								if(r)	{
									hextemp = keyfound.GetBase16();
									printf("[+] Thread Key found privkey %s   \n",hextemp);
									point_found = secp->ComputePublicKey(&keyfound);
									aux_c = secp->GetPublicKeyHex(OriginalPointsBSGScompressed[k],point_found);
									printf("[+] Publickey %s\n",aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
									WaitForSingleObject(write_keys, INFINITE);
#else
									pthread_mutex_lock(&write_keys);
#endif

									filekey = fopen("KEYFOUNDKEYFOUND.txt","a");
									if(filekey != NULL)	{
										fprintf(filekey,"Key found privkey %s\nPublickey %s\n",hextemp,aux_c);
										fclose(filekey);
									}
									free(hextemp);
								free(aux_c);
#if defined(_WIN64) && !defined(__CYGWIN__)
								ReleaseMutex(write_keys);
#else
								pthread_mutex_unlock(&write_keys);
#endif

								bsgs_found[k] = 1;
								notify_key_found(&keyfound);
								notify_key_found(&keyfound);
								salir = 1;
								for(l = 0; l < bsgs_point_number && salir; l++)	{
									salir &= bsgs_found[l];
								}
								if(salir)	{
									printf("All points were found\n");
									exit(EXIT_FAILURE);
								}
							} //End if second check
						}//End if first check
						
					}// For for pts variable
						
						// Next start point (startP += (bsSize*GRP_SIZE).G)
						
						pp = startP;
						dy.ModSub(&_2GSn.y,&pp.y);

						_s.ModMulK1(&dy,&dx[i + 1]);
						_p.ModSquareK1(&_s);

						pp.x.ModNeg();
						pp.x.ModAdd(&_p);
						pp.x.ModSub(&_2GSn.x);

						pp.y.ModSub(&_2GSn.x,&pp.x);
						pp.y.ModMulK1(&_s);
						pp.y.ModSub(&_2GSn.y);
						startP = pp;
						
						j++;
					}//while all the aMP points
			}// End if 
		}
		steps[thread_number]+=2;	
	}while(1);
	ends[thread_number] = 1;
	return NULL;
}


/* This function takes in three parameters:

buffer: a pointer to a char array where the minikey will be stored.
rawbuffer: a pointer to a char array that contains the raw data.
length: an integer representing the length of the raw data.
The function is designed to convert the raw data using a lookup table (Ccoinbuffer) and store the result in the buffer. 
*/
void set_minikey(char *buffer,char *rawbuffer,int length)	{
	for(int i = 0;  i < length; i++)	{
		buffer[i] = Ccoinbuffer[(uint8_t)rawbuffer[i]];
	}
}

/* This function takes in three parameters:

buffer: a pointer to a char array where the minikey will be stored.
rawbuffer: a pointer to a char array that contains the raw data.
index: an integer representing the index of the raw data array to be incremented.
The function is designed to increment the value at the specified index in the raw data array,
and update the corresponding value in the buffer using a lookup table (Ccoinbuffer).
If the value at the specified index exceeds 57, it is reset to 0x00 and the function recursively
calls itself to increment the value at the previous index, unless the index is already 0, in which
case the function returns false. The function returns true otherwise. 
*/

bool increment_minikey_index(char *buffer,char *rawbuffer,int index)	{
	if(rawbuffer[index] < 57){
		rawbuffer[index]++;
		buffer[index] = Ccoinbuffer[(uint8_t)rawbuffer[index]];
	}
	else	{
		rawbuffer[index] = 0x00;
		buffer[index] = Ccoinbuffer[0];
		if(index>0)	{
			return increment_minikey_index(buffer,rawbuffer,index-1);
		}
		else	{
			return false;
		}
	}
	return true;
}

/* This function takes in a single parameter:

rawbuffer: a pointer to a char array that contains the raw data.
The function is designed to increment the values in the raw data array
using a lookup table (minikeyN), while also handling carry-over to the
previous element in the array if necessary. The maximum number of iterations
is limited by minikey_n_limit. 


*/
void increment_minikey_N(char *rawbuffer)	{
	int i = 20,j = 0;
	while( i > 0 && j < minikey_n_limit)	{
		rawbuffer[i] = rawbuffer[i] + minikeyN[i];
		if(rawbuffer[i] > 57)	{	 // Handling carry-over if value exceeds 57
			rawbuffer[i] = rawbuffer[i] % 58;
			rawbuffer[i-1]++;
		}
		i--;
		j++;
	}
}


#define BUFFMINIKEY(buff,src) \
(buff)[ 0] = (uint32_t)src[ 0] << 24 | (uint32_t)src[ 1] << 16 | (uint32_t)src[ 2] << 8 | (uint32_t)src[ 3]; \
(buff)[ 1] = (uint32_t)src[ 4] << 24 | (uint32_t)src[ 5] << 16 | (uint32_t)src[ 6] << 8 | (uint32_t)src[ 7]; \
(buff)[ 2] = (uint32_t)src[ 8] << 24 | (uint32_t)src[ 9] << 16 | (uint32_t)src[10] << 8 | (uint32_t)src[11]; \
(buff)[ 3] = (uint32_t)src[12] << 24 | (uint32_t)src[13] << 16 | (uint32_t)src[14] << 8 | (uint32_t)src[15]; \
(buff)[ 4] = (uint32_t)src[16] << 24 | (uint32_t)src[17] << 16 | (uint32_t)src[18] << 8 | (uint32_t)src[19]; \
(buff)[ 5] = (uint32_t)src[20] << 24 | (uint32_t)src[21] << 16 | 0x8000; \
(buff)[ 6] = 0; \
(buff)[ 7] = 0; \
(buff)[ 8] = 0; \
(buff)[ 9] = 0; \
(buff)[10] = 0; \
(buff)[11] = 0; \
(buff)[12] = 0; \
(buff)[13] = 0; \
(buff)[14] = 0; \
(buff)[15] = 0xB0;	//176 bits => 22 BYTES


void sha256sse_22(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3)	{
  uint32_t b0[16];
  uint32_t b1[16];
  uint32_t b2[16];
  uint32_t b3[16];
  BUFFMINIKEY(b0, src0);
  BUFFMINIKEY(b1, src1);
  BUFFMINIKEY(b2, src2);
  BUFFMINIKEY(b3, src3);
  sha256sse_1B(b0, b1, b2, b3, dst0, dst1, dst2, dst3);
}


#define BUFFMINIKEYCHECK(buff,src) \
(buff)[ 0] = (uint32_t)src[ 0] << 24 | (uint32_t)src[ 1] << 16 | (uint32_t)src[ 2] << 8 | (uint32_t)src[ 3]; \
(buff)[ 1] = (uint32_t)src[ 4] << 24 | (uint32_t)src[ 5] << 16 | (uint32_t)src[ 6] << 8 | (uint32_t)src[ 7]; \
(buff)[ 2] = (uint32_t)src[ 8] << 24 | (uint32_t)src[ 9] << 16 | (uint32_t)src[10] << 8 | (uint32_t)src[11]; \
(buff)[ 3] = (uint32_t)src[12] << 24 | (uint32_t)src[13] << 16 | (uint32_t)src[14] << 8 | (uint32_t)src[15]; \
(buff)[ 4] = (uint32_t)src[16] << 24 | (uint32_t)src[17] << 16 | (uint32_t)src[18] << 8 | (uint32_t)src[19]; \
(buff)[ 5] = (uint32_t)src[20] << 24 | (uint32_t)src[21] << 16 | (uint32_t)src[22] << 8 | 0x80; \
(buff)[ 6] = 0; \
(buff)[ 7] = 0; \
(buff)[ 8] = 0; \
(buff)[ 9] = 0; \
(buff)[10] = 0; \
(buff)[11] = 0; \
(buff)[12] = 0; \
(buff)[13] = 0; \
(buff)[14] = 0; \
(buff)[15] = 0xB8;	//184 bits => 23 BYTES

void sha256sse_23(uint8_t *src0, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *dst0, uint8_t *dst1, uint8_t *dst2, uint8_t *dst3)	{
  uint32_t b0[16];
  uint32_t b1[16];
  uint32_t b2[16];
  uint32_t b3[16];
  BUFFMINIKEYCHECK(b0, src0);
  BUFFMINIKEYCHECK(b1, src1);
  BUFFMINIKEYCHECK(b2, src2);
  BUFFMINIKEYCHECK(b3, src3);
  sha256sse_1B(b0, b1, b2, b3, dst0, dst1, dst2, dst3);
}

void menu() {
	printf("\n");
	printf("===============================================================\n");
	printf("  TrueCollider Search Modes + Binary Fuse Filters\n");
	printf("  Developed & Modified by TrueScent\n");
	printf("===============================================================\n\n");
	printf("USAGE:\n");
	printf("  keyhunt -m <mode> -f <target_file> [options]\n\n");

	printf("===============================================================\n");
	printf("  SEARCH MODES (-m)\n");
	printf("===============================================================\n\n");

	printf("  address\n");
	printf("    The default mode. Generates random private keys (or walks a range),\n");
	printf("    derives the public key and address, then checks against target file.\n");
	printf("    Supports: -c btc/eth/ltc/doge/xrp/bch/btg/etc/troot/all\n");
	printf("    Supports: -l compress/uncompress/both, -r range, -x modes\n");
	printf("    Supports: -p path -D count (BIP-32 derivation for each key)\n");
	printf("    Supports: -N[url] node balance check (own node or public API)\n");
	printf("    Input file: one address per line (format depends on currency)\n\n");

	printf("    How it works:\n");
	printf("      1. Load target addresses into binary fuse filter + sorted array\n");
	printf("      2. Each thread generates candidate private keys\n");
	printf("      3. Derives public key -> address (SHA256 + RIPEMD160 for BTC)\n");
	printf("      4. Checks address against filter (fast rejection) then binary search\n");
	printf("      5. Found key is written to KEYFOUNDKEYFOUND.txt\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m address -f targets.txt -x chaos -t 8\n\n");

	printf("  pubkey2addr\n");
	printf("    Random key generation with address matching. Similar to address mode\n");
	printf("    but specifically designed for random-only key generation (no sequential\n");
	printf("    walking). Defaults to -x random. Always generates random keys regardless\n");
	printf("    of range flags.\n");
	printf("    Supports: -c btc/eth/ltc/doge/xrp/bch/btg/etc/troot/all\n");
	printf("    Supports: -l compress/uncompress/both, -x modes\n");
	printf("    Input file: one address per line (format depends on currency)\n\n");

	printf("    How it works:\n");
	printf("      1. Load target addresses into binary fuse filter\n");
	printf("      2. Each thread picks random private keys across full key space\n");
	printf("      3. Derives public key -> BTC or ETH address\n");
	printf("      4. Checks address against filter + binary search on match\n");
	printf("      5. Found key is written to KEYFOUNDKEYFOUND.txt\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m pubkey2addr -f targets.txt -x auto -t 4\n");
	printf("      keyhunt -m pubkey2addr -c eth -f eth_targets.txt -t 8\n\n");

	printf("  rmd160\n");
	printf("    Searches for private keys whose RIPEMD-160 hash of the public key\n");
	printf("    matches entries in the target file. Useful when you have raw hash\n");
	printf("    values rather than addresses.\n");
	printf("    Supports: -c btc/eth/ltc/doge/xrp/bch/btg/etc/all\n");
	printf("    Supports: -l compress/uncompress/both, -r range, -x modes, -e\n");
	printf("    Supports: -p path -D count (BIP-32 derivation for each key)\n");
	printf("    Input file: one 40-char hex RIPEMD-160 hash per line\n\n");

	printf("    How it works:\n");
	printf("      1. Load target RIPEMD-160 hashes into binary fuse filter\n");
	printf("      2. Generates candidate keys and computes hash160 directly\n");
	printf("      3. Checks against filter without base58 encoding (faster)\n");
	printf("      4. On match, prints the full private key and address\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m rmd160 -f hashes.rmd -l compress -x gravity -t 8\n\n");

	printf("  xpoint\n");
	printf("    Searches for private keys by matching the x-coordinate of the\n");
	printf("    derived public key against public key data in the target file.\n");
	printf("    Used when you have the full public key or just the x-coordinate.\n");
	printf("    Supports: -r range, -x modes\n");
	printf("    Input file: one hex public key per line (64-char x-only,\n");
	printf("               66-char compressed, or 130-char uncompressed with 04 prefix)\n\n");

	printf("    How it works:\n");
	printf("      1. Parse public keys from file, extract x-coordinates\n");
	printf("      2. Generate candidate keys and compute public keys\n");
	printf("      3. Compare x-coordinate against the sorted target array\n");
	printf("      4. On match, prints the full private key\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m xpoint -f pubkeys.txt -t 8\n\n");

	printf("  bsgs\n");
	printf("    Baby-Step Giant-Step algorithm. Solves the elliptic curve discrete\n");
	printf("    log problem when a public key is known. Precomputes a table of baby\n");
	printf("    steps (public key points) then walks giant steps to find the match.\n");
	printf("    Best for puzzles where a public key is given and you need the\n");
	printf("    corresponding private key. Uses 3-tier bloom filter for lookups.\n");
	printf("    Supports: -B sequential/backward/both/random/dance, -x modes,\n");
	printf("              -k factor, -S save/load, -n table size\n");
	printf("    Input file: compressed (66-char) or uncompressed (130-char) public keys\n\n");

	printf("    How it works:\n");
	printf("      1. Build baby-step table from target public keys (x-coordinates)\n");
	printf("      2. Build 3-tier binary fuse/bloom filter from table\n");
	printf("      3. Walk giant steps through key space, checking each against table\n");
	printf("      4. On bloom hit, verify with second/third check for confirmation\n");
	printf("      5. Found key written to KEYFOUNDKEYFOUND.txt\n\n");

	printf("    BSGS search modes (-B):\n");
	printf("      sequential - walk forward from start\n");
	printf("      backward   - walk backward from end\n");
	printf("      both       - alternate top/bottom of range\n");
	printf("      random     - random giant steps (default)\n");
	printf("      dance      - triple: top/bottom/random alternation\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m bsgs -f pubkeys.txt -B sequential -n 0x1000000 -t 4\n");
	printf("      keyhunt -m bsgs -f pubkeys.txt -x auto -t 8\n\n");

	printf("  vanity\n");
	printf("    Searches for a private key whose address starts with a specific\n");
	printf("    prefix pattern. Loads a vanity target via -v flag.\n");
	printf("    Supports: -v target, -e endomorphism, -x modes\n\n");

	printf("    How it works:\n");
	printf("      1. Parse the target prefix and build matching rules\n");
	printf("      2. Generate candidate keys and derive addresses\n");
	printf("      3. Check if the address starts with the target prefix\n");
	printf("      4. On match, prints the private key and full address\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m vanity -v 1Cool -e -t 8\n\n");

	printf("  minikeys\n");
	printf("    Generates and tests Bitcoin minikeys (short private keys starting\n");
	printf("    with 'S'). Each valid minikey starts with 'S' and its SHA256 hash\n");
	printf("    begins with 0x00. Useful for creating short, memorable keys.\n");
	printf("    Supports: -C base string, -8 custom alphabet, -f targets\n\n");

	printf("    How it works:\n");
	printf("      1. Generate candidate minikey strings\n");
	printf("      2. Validate: starts with S, SHA256 first byte is 0x00\n");
	printf("      3. Derive private key -> public key -> address\n");
	printf("      4. Check against target address file\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m minikeys -f targets.txt -t 4\n");
	printf("      keyhunt -m minikeys -C SRPqx8QiwnW4WNWnTVa2W5 -f targets.txt\n\n");

	printf("  mnemonic\n");
	printf("    Generates random BIP-39 mnemonics, validates their checksum,\n");
	printf("    derives seed via PBKDF2-HMAC-SHA512 (2048 rounds), then derives\n");
	printf("    private keys via BIP-32 along three standard paths:\n");
	printf("      BIP-44  m/44'/0'/0'/0/N  Legacy P2PKH (1...)\n");
	printf("      BIP-49  m/49'/0'/0'/0/N  Wrapped SegWit (3...)\n");
	printf("      BIP-84  m/84'/0'/0'/0/N  Native SegWit (bc1...)\n");
	printf("    Supports all 10 BIP-39 languages and ETH via -W flag.\n");
	printf("    Supports: -w word count, -W ETH, -L language, -D index count\n\n");

	printf("    How it works:\n");
	printf("      1. Generate random entropy (128-256 bits)\n");
	printf("      2. SHA-256 checksum, encode as 12-24 BIP-39 words\n");
	printf("      3. Validate mnemonic checksum (rejects invalid)\n");
	printf("      4. PBKDF2 -> seed -> BIP-32 master key\n");
	printf("      5. Derive child keys along each path\n");
	printf("      6. Compute address for each derived key\n");
	printf("      7. Check against binary fuse filter of target addresses\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4\n");
	printf("      keyhunt -m mnemonic -W -L all -f eth_targets.txt -t 8\n");
	printf("      keyhunt -m mnemonic -D 50 -f targets.txt -t 8\n\n");

	printf("  poetry\n");
	printf("    Generates random poetry words, encodes them as hex private keys,\n");
	printf("    then derives public keys and checks against target addresses.\n");
	printf("    Uses a custom word list (tests/poetry.txt, 547 words).\n");
	printf("    Supports: -f targets, -x modes\n\n");

	printf("    How it works:\n");
	printf("      1. Pick random poetry words (3 words per group)\n");
	printf("      2. Decode word indices into hex private key\n");
	printf("      3. Derive public key -> address\n");
	printf("      4. Check against binary fuse filter of targets\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m poetry -f targets.txt -t 4\n\n");

	printf("  brainwallet\n");
	printf("    Generates passphrases from a word list (tests/brainwalletwords.txt,\n");
	printf("    352 words), then applies SHA-256 to derive private keys. Tests\n");
	printf("    multiple mutations of each passphrase: double-SHA256, number\n");
	printf("    suffixes (1,123,0,2024,69,420,7,13,99), leet speak substitutions,\n");
	printf("    capitalization, ALL CAPS, and various separators.\n");
	printf("    Supports: -w word count, -f targets, -x modes\n\n");

	printf("    How it works:\n");
	printf("      1. Pick 1-24 random words from wordlist (or use -w count)\n");
	printf("      2. Apply SHA256 and double-SHA256 to the passphrase\n");
	printf("      3. Try number suffixes, leet speak, caps, separators\n");
	printf("      4. Derive public key -> address for each variant\n");
	printf("      5. Check each against binary fuse filter of targets\n\n");

	printf("    Example:\n");
	printf("      keyhunt -m brainwallet -f targets.txt -t 8\n");
	printf("      keyhunt -m brainwallet -w 3 -f targets.txt -t 4\n\n");

	printf("===============================================================\n");
	printf("  OPTIONS\n");
	printf("===============================================================\n\n");

	printf("REQUIRED:\n");
	printf("  -m mode      Search mode (see above). Default: address\n");
	printf("  -f file      Input file with target data\n\n");

	printf("CRYPTO:\n");
	printf("  -c crypto    btc, eth, ltc, doge, xrp, sol, bch, btg, etc, troot, all. Default: btc\n");
	printf("               btc   = Bitcoin 1.../3.../bc1q... addresses\n");
	printf("               eth   = Ethereum 0x... addresses\n");
	printf("               ltc   = Litecoin L... addresses\n");
	printf("               doge  = Dogecoin D... addresses\n");
	printf("               xrp   = XRP (Ripple) r... addresses\n");
	printf("               sol   = Solana (base58) addresses\n");
	printf("               bch   = Bitcoin Cash addresses\n");
	printf("               btg   = Bitcoin Gold G... addresses\n");
	printf("               etc   = Ethereum Classic 0x... addresses\n");
	printf("               troot = Taproot (P2TR) bc1p... addresses\n");
	printf("               all   = All supported currencies simultaneously\n");
	printf("               Applies to: address, rmd160, vanity modes\n\n");

	printf("  -N[url]      Node balance check when key is found (requires curl)\n");
	printf("               Without URL: uses public APIs (blockstream.info, etherscan)\n");
	printf("               With URL: connects to your own Bitcoin Core node via RPC\n");
	printf("               Format: http://user:pass@host:port (default: http://127.0.0.1:8332)\n");
	printf("               Supports: BTC (own node or public API), ETH, LTC, ETC (public APIs)\n\n");

	printf("RANGE:\n");
	printf("  -r SR:EN     Hex range StartRange:EndRange\n");
	printf("               EndRange optional (defaults to curve order)\n");
	printf("  -T timestamp Unix timestamp (seconds since epoch). Sets search range\n");
	printf("               to ~4B keys centered around that timestamp value.\n");
	printf("               Useful for puzzles where key was generated at a known time.\n");
	printf("  -Z bytes     Strip N leading zero bytes from key (requires -b).\n");
	printf("               Reduces search space for padded keys.\n");
	printf("               Example: -b 72 -Z 6 strips 6 zero bytes, searches ~16M keys\n");
	printf("  -n number    Sequential keys per cycle / BSGS baby-step table size\n");
	printf("               Must be divisible by 1024 for BSGS mode\n");
	printf("  -b bits      Bit range - only test keys with this many bits\n\n");

	printf("BSGS OPTIONS (-m bsgs):\n");
	printf("  -B mode      BSGS giant-step strategy:\n");
	printf("                 sequential - walk forward linearly\n");
	printf("               (see mode descriptions above for full details)\n");
	printf("  -k value     K factor multiplies M (baby-step count). More speed,\n");
	printf("               but uses proportionally more RAM\n");
	printf("  -S           Save/load BSGS bloom filters and baby-step table to disk.\n");
	printf("               Files: keyhunt_bsgs_*.blm, keyhunt_bsgs_*.tbl\n");
	printf("  -z value     Bloom filter size multiplier (>= 1). Default: 1\n\n");

	printf("MNEMONIC OPTIONS (-m mnemonic):\n");
	printf("  -w count     Word count: 0=random, 12, 15, 18, 21, 24. Default: 0\n");
	printf("  -W           Check ETH addresses (keccak256) instead of BTC\n");
	printf("  -L lang      BIP39 language. Default: english\n");
	printf("               english, spanish, french, italian, czech, portuguese,\n");
	printf("               japanese, korean, chinese_simplified, chinese_traditional\n");
	printf("               Use 'all' to cycle through all 10 languages\n");
	printf("  -D count     Address indices per derivation path (1-100). Default: 1\n");
	printf("               Total checks per mnemonic = 3 paths x -D value\n\n");

	printf("VANITY OPTIONS (-m vanity):\n");
	printf("  -v value     Target vanity prefix (e.g. \"1Cool\", \"bc1qabc\")\n\n");

	printf("MINIKEY OPTIONS (-m minikeys):\n");
	printf("  -C string    Base minikey string (22 char). Starts with S\n");
	printf("  -8 alpha     Custom base58 alphabet (58 chars)\n\n");

	printf("BRAINWALLET OPTIONS (-m brainwallet):\n");
	printf("  -w count     0=random, 1,2,3,6,9,12,15,18,21,24. Default: 0\n");
	printf("  Mutations per passphrase: SHA256, double-SHA256, number suffixes,\n");
	printf("  leet speak, capitalize, ALL CAPS, separators (space/dash/dot/none)\n\n");

	printf("ADDRESS FILTER:\n");
	printf("  -l look      compress, uncompress, both. Default: both\n");
	printf("               Applies to: address, rmd160, pubkey2addr modes\n\n");

	printf("DERIVATION PATH (-m address / -m rmd160):\n");
	printf("  -p path      BIP-32 derivation path (e.g. m/44'/0'/0'/0, m/84'/0'/0'/0)\n");
	printf("               Each random key is used as a BIP-32 master key.\n");
	printf("               Child keys are derived along the path for indices 0 to -D-1.\n");
	printf("  -D count     Child key indices to check per base key (1-100). Default: 1\n");
	printf("               Example: -p m/84'/0'/0'/0 -D 10 checks indices 0-9\n");
	printf("               Supports hardened: ' or H or h suffix (e.g. 44')\n");
	printf("               Applies to: address, rmd160 modes\n\n");

	printf("THREADING & SEARCH PATTERNS:\n");
	printf("  -t tn        Number of threads. Default: 1\n");
	printf("  -x mode      Key generation / search pattern:\n");
	printf("                 sequential - Linear walk from start to end of range\n");
	printf("                 random     - Random key selection across full range\n");
	printf("                 chaos      - Logistic map chaotic sequence (r=3.99999)\n");
	printf("                 gravity    - Adaptive: clusters around previously found keys\n");
	printf("                 spiral     - Archimedean spiral from range midpoint\n");
	printf("                 reverse    - Inverted BSGS baby/giant step roles\n");
	printf("                 auto       - Cycles: spiral->chaos->gravity->reverse\n");
	printf("               Works with ALL modes including BSGS.\n\n");

	printf("PERFORMANCE:\n");
	printf("  -e           Enable GLV endomorphism (3x speedup for address/rmd160/vanity)\n");
	printf("  -I stride    Stride value for address/rmd160/xpoint modes\n\n");

	printf("OUTPUT:\n");
	printf("  -s seconds   Stats output interval in seconds. 0=off. Default: 30\n");
	printf("  -q           Quiet mode - suppress per-thread output\n");
	printf("  -V           Verbose mode - show full derivation path and chain code\n");
	printf("  -M           Matrix screen effect (reduces performance)\n");
	printf("  -6           Skip SHA-256 checksum validation on cached data files\n\n");

	printf("===============================================================\n");
	printf("  FILE FORMATS\n");
	printf("===============================================================\n\n");

	printf("  Address targets:    One BTC address (base58) or ETH (0x...) per line\n");
	printf("  RMD160 targets:     One 40-char hex RIPEMD-160 hash per line\n");
	printf("  Public key targets: One hex public key per line (64/66/130 chars)\n");
	printf("  Vanity targets:     Via -v flag, not file\n\n");

	printf("===============================================================\n");
	printf("  EXAMPLES\n");
	printf("===============================================================\n\n");

	printf("  keyhunt -m address -f targets.txt -t 8\n");
	printf("    Search for BTC address matches using 8 threads\n\n");

	printf("  keyhunt -m pubkey2addr -f targets.txt -x auto -t 4\n");
	printf("    Random key generation, BTC addresses, auto search pattern\n\n");

	printf("  keyhunt -m pubkey2addr -c eth -f eth_targets.txt -t 8\n");
	printf("    Random key generation, ETH addresses, 8 threads\n\n");

	printf("  keyhunt -m address -f targets.txt -x chaos -e -t 8\n");
	printf("    Address search with chaos pattern and endomorphism\n\n");

	printf("  keyhunt -m rmd160 -f hashes.rmd -l compress -x gravity -t 8\n");
	printf("    RMD160 hash search with compressed keys and gravity pattern\n\n");

	printf("  keyhunt -m xpoint -f pubkeys.txt -t 8\n");
	printf("    X-coordinate matching against public key file\n\n");

	printf("  keyhunt -m bsgs -f pubkeys.txt -B sequential -n 0x1000000 -t 4\n");
	printf("    BSGS puzzle solver, sequential giant steps, 1M baby steps\n\n");

	printf("  keyhunt -m bsgs -f pubkeys.txt -x auto -S -t 8\n");
	printf("    BSGS with auto search cycling, save bloom filters to disk\n\n");

	printf("  keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4\n");
	printf("    24-word English mnemonics, check BIP-44/49/84\n\n");

	printf("  keyhunt -m mnemonic -W -L all -f eth_targets.txt -t 8\n");
	printf("    Mnemonics in all 10 languages, ETH address matching\n\n");

	printf("  keyhunt -m mnemonic -D 50 -f targets.txt -t 8\n");
	printf("    Check 50 address indices per path (150 total per mnemonic)\n\n");

	printf("  keyhunt -m vanity -v 1Cool -e -t 8\n");
	printf("    Vanity search for 1Cool... with endomorphism\n\n");

	printf("  keyhunt -m address -c troot -f troot_targets.txt -t 8\n");
	printf("    Taproot (P2TR) bc1p... address search\n\n");

	printf("  keyhunt -m address -c ltc -f ltc_targets.txt -t 8\n");
	printf("    Litecoin L... address search\n\n");

	printf("  keyhunt -m address -c bch -f bch_targets.txt -t 8\n");
	printf("    Bitcoin Cash address search\n\n");

	printf("  keyhunt -m address -c btg -f btg_targets.txt -t 8\n");
	printf("    Bitcoin Gold G... address search\n\n");

	printf("  keyhunt -m address -c etc -f etc_targets.txt -t 8\n");
	printf("    Ethereum Classic 0x... address search\n\n");

	printf("  keyhunt -m address -c all -f all_targets.txt -t 8\n");
	printf("    Search BTC + ETH + LTC + BCH + BTG + ETC simultaneously\n\n");

	printf("  keyhunt -m rmd160 -c ltc -f ltc_hashes.rmd -t 8\n");
	printf("    Litecoin RMD160 hash search\n\n");

	printf("  keyhunt -m address -c troot -p \"m/86'/0'/0'/0\" -D 10 -f troot_targets.txt -V -t 8\n");
	printf("    Taproot search with BIP-86 derivation, verbose output\n\n");

	printf("  keyhunt -m address -p \"m/84'/0'/0'/0\" -D 20 -f targets.txt -V -t 8\n");
	printf("    BTC address search with BIP-84 derivation, 20 indices, verbose\n\n");

	printf("  keyhunt -m rmd160 -p \"m/44'/0'/0'/0\" -D 10 -f hashes.rmd -t 8\n");
	printf("    RMD160 search with BIP-44 derivation path\n\n");

	printf("  keyhunt -m brainwallet -w 3 -f targets.txt -t 8\n");
	printf("    Brainwallet with 3-word passphrases\n\n");

	printf("  keyhunt -m minikeys -C SRPqx8QiwnW4WNWnTVa2W5 -f targets.txt\n");
	printf("    Minikeys from custom base string\n\n");

	printf("  keyhunt -m address -f targets.txt -r 1:FFFF -t 8\n");
	printf("    Search first 65536 keys in range\n\n");

	printf("  keyhunt -m address -f targets.txt -b 40 -t 8\n");
	printf("    Only test keys with 40 bits (for short-key puzzles)\n\n");

	printf("  keyhunt -m address -f targets.txt -T 1609459200 -t 8\n");
	printf("    Search around Jan 1 2021 timestamp (~4B keys range)\n\n");

	printf("  keyhunt -m address -f targets.txt -T 1609459200 -b 32 -t 8\n");
	printf("    Timestamp + bit range for tighter search window\n\n");

	printf("  keyhunt -m address -f targets.txt -T 1421345234 -b 72 -t 8\n");
	printf("    Bitcoin puzzle search using funding timestamp (Jan 15 2015)\n\n");

	printf("  keyhunt -m address -f targets.txt -b 72 -t 8 -x auto\n");
	printf("    Puzzle 72 search with auto pattern cycling\n\n");

	printf("  keyhunt -m pubkey2addr -f targets.txt -q -s 10 -t 4\n");
	printf("    Quiet mode, stats every 10 seconds\n\n");

	printf("===============================================================\n");
	printf("  BITCOIN PUZZLE SEARCH (72-160)\n");
	printf("===============================================================\n\n");

	printf("  For puzzles 72-160, use timestamp 1421345234 (Jan 15 2015 18:07 UTC)\n");
	printf("  All puzzles funded in single tx: 08389f34c98c606322740c0be6a7125d9860bb8d5cb182c02f98461e5fa6cd15\n");
	printf("  Block: 339085, 256 outputs, puzzles 1-256\n\n");

	printf("  Search strategy:\n");
	printf("    1. Start with lower bit puzzles (72-80) - faster to search\n");
	printf("    2. Use -T 1421345234 to constrain by funding timestamp\n");
	printf("    3. Combine with -x auto for best coverage\n");
	printf("    4. If you find ONE key, derive all others via BIP-32\n\n");

	printf("  Example puzzle searches:\n");
	printf("    keyhunt -m address -f tests/unsolvedpuzzles.txt -b 72 -T 1421345234 -t 8 -x auto\n");
	printf("    keyhunt -m address -f tests/unsolvedpuzzles.txt -b 80 -T 1421345234 -t 8\n");
	printf("    keyhunt -m address -f tests/unsolvedpuzzles.txt -b 100 -T 1421345234 -t 8\n");
	printf("    keyhunt -m address -f tests/unsolvedpuzzles.txt -b 128 -T 1421345234 -t 8\n\n");

	printf("  Run all puzzles 72-160 batch:\n");
	printf("    bash run_puzzle_search.sh\n\n");

	printf("===============================================================\n");
	printf("  TrueCollider Search Modes + Binary Fuse Filters\n");
	printf("  Developed & Modified by TrueScent\n");
	printf("  Donations:\n");
	printf("    BTC:  1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h\n");
	printf("    Tips to Iceland: bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at\n");
	printf("===============================================================\n\n");
	exit(EXIT_FAILURE);
}

bool vanityrmdmatch(unsigned char *rmdhash)	{
	bool r = false;
	int i,j,cmpA,cmpB,result;
	result = bloom_check(vanity_bloom,rmdhash,vanity_rmd_minimun_bytes_check_length);
	switch(result)	{
		case -1:
			fprintf(stderr,"[E] Bloom is not initialized\n");
			exit(EXIT_FAILURE);
		break;
		case 1:
			for(i = 0; i < vanity_rmd_targets && !r;i++)	{
				for(j = 0; j < vanity_rmd_limits[i] && !r; j++)	{
					cmpA = memcmp(vanity_rmd_limit_values_A[i][j],rmdhash,20);
					cmpB = memcmp(vanity_rmd_limit_values_B[i][j],rmdhash,20);
					if(cmpA <= 0 && cmpB >= 0)	{
						//if(FLAGDEBUG ) printf("\n\n[D] cmpA = %i, cmpB = %i \n\n",cmpA,cmpB);
						r = true;
					}
				}
			}
		break;
		default:
			r = false;
		break;
	}
	return r;
}

void writevanitykey(bool compressed,Int *key)	{
	Point publickey;
	FILE *keys;
	char *hextemp,*hexrmd,public_key_hex[131],address[50],rmdhash[20];
	hextemp = key->GetBase16();
	publickey = secp->ComputePublicKey(key);
	secp->GetPublicKeyHex(compressed,publickey,public_key_hex);
	
	secp->GetHash160(P2PKH,compressed,publickey,(uint8_t*)rmdhash);
	hexrmd = tohex(rmdhash,20);
	rmd160toaddress_dst(rmdhash,address);
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(write_keys, INFINITE);
#else
	pthread_mutex_lock(&write_keys);
#endif
	keys = fopen("VANITYKEYFOUND.txt","a+");
	if(keys != NULL)	{
		fprintf(keys,"Vanity Private Key: %s\npubkey: %s\nAddress %s\nrmd160 %s\n",hextemp,public_key_hex,address,hexrmd);
		fclose(keys);
	}
	printf("\nVanity Private Key: %s\npubkey: %s\nAddress %s\nrmd160 %s\n",hextemp,public_key_hex,address,hexrmd);
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(write_keys);
#else
	pthread_mutex_unlock(&write_keys);
#endif
	free(hextemp);
	free(hexrmd);
}


int addvanity(char *target)	{
	unsigned char raw_value_A[50],raw_value_B[50];
	char target_copy[50];
	int stringsize,targetsize,j,r = 0;
	size_t raw_value_length;
	int values_A_size = 0,values_B_size = 0,minimun_bytes;
	raw_value_length = 50;
	targetsize = strlen(target);
	stringsize = targetsize;
	memset(raw_value_A,0,50);
	memset(target_copy,0,50);
	if(targetsize >= 30 )	{
		return 0;
	}
	memcpy(target_copy,target,targetsize);
	j = 0;
	vanity_address_targets = (char**)  realloc(vanity_address_targets,(vanity_rmd_targets+1) * sizeof(char*));
	vanity_address_targets[vanity_rmd_targets] = NULL;
	checkpointer((void *)vanity_address_targets,__FILE__,"realloc","vanity_address_targets" ,__LINE__ -1 );
	vanity_rmd_limits = (int*) realloc(vanity_rmd_limits,(vanity_rmd_targets+1) * sizeof(int));
	vanity_rmd_limits[vanity_rmd_targets] = 0;
	checkpointer((void *)vanity_rmd_limits,__FILE__,"realloc","vanity_rmd_limits" ,__LINE__ -1 );
	vanity_rmd_limit_values_A = (uint8_t***)realloc(vanity_rmd_limit_values_A,(vanity_rmd_targets+1) * sizeof(unsigned char *));
	checkpointer((void *)vanity_rmd_limit_values_A,__FILE__,"realloc","vanity_rmd_limit_values_A" ,__LINE__ -1 );
	vanity_rmd_limit_values_A[vanity_rmd_targets] = NULL;
	vanity_rmd_limit_values_B = (uint8_t***)realloc(vanity_rmd_limit_values_B,(vanity_rmd_targets+1) * sizeof(unsigned char *));
	checkpointer((void *)vanity_rmd_limit_values_B,__FILE__,"realloc","vanity_rmd_limit_values_B" ,__LINE__ -1 );
	vanity_rmd_limit_values_B[vanity_rmd_targets] = NULL;
	do	{
		raw_value_length = 50;
		b58tobin(raw_value_A,&raw_value_length,target_copy,stringsize);
		if(raw_value_length < 25)	{
			target_copy[stringsize] = '1';
			stringsize++;
		}
		if(raw_value_length == 25)	{
			b58tobin(raw_value_A,&raw_value_length,target_copy,stringsize);
			
			vanity_rmd_limit_values_A[vanity_rmd_targets] = (uint8_t**)realloc(vanity_rmd_limit_values_A[vanity_rmd_targets],(j+1) * sizeof(unsigned char *));
			checkpointer((void *)vanity_rmd_limit_values_A[vanity_rmd_targets],__FILE__,"realloc","vanity_rmd_limit_values_A" ,__LINE__ -1 );
			vanity_rmd_limit_values_A[vanity_rmd_targets][j] = (uint8_t*)calloc(20,1);
			checkpointer((void *)vanity_rmd_limit_values_A[vanity_rmd_targets][j],__FILE__,"realloc","vanity_rmd_limit_values_A" ,__LINE__ -1 );
			
			memcpy(vanity_rmd_limit_values_A[vanity_rmd_targets][j] ,raw_value_A +1,20);
			
			j++;	
			values_A_size = j;
			target_copy[stringsize] = '1';
			stringsize++;
		}	
	}while(raw_value_length <= 25);
	
	stringsize = targetsize;
	memset(raw_value_B,0,50);
	memset(target_copy,0,50);
	memcpy(target_copy,target,targetsize);

	j = 0;
	do	{
		raw_value_length = 50;
		b58tobin(raw_value_B,&raw_value_length,target_copy,stringsize);
		if(raw_value_length < 25)	{
			target_copy[stringsize] = 'z';
			stringsize++;
		}
		if(raw_value_length == 25)	{
			
			b58tobin(raw_value_B,&raw_value_length,target_copy,stringsize);
			vanity_rmd_limit_values_B[vanity_rmd_targets] = (uint8_t**)realloc(vanity_rmd_limit_values_B[vanity_rmd_targets],(j+1) * sizeof(unsigned char *));
			checkpointer((void *)vanity_rmd_limit_values_B[vanity_rmd_targets],__FILE__,"realloc","vanity_rmd_limit_values_B" ,__LINE__ -1 );
			checkpointer((void *)vanity_rmd_limit_values_B[vanity_rmd_targets],__FILE__,"realloc","vanity_rmd_limit_values_B" ,__LINE__ -1 );
			vanity_rmd_limit_values_B[vanity_rmd_targets][j] = (uint8_t*)calloc(20,1);
			checkpointer((void *)vanity_rmd_limit_values_B[vanity_rmd_targets][j],__FILE__,"calloc","vanity_rmd_limit_values_B" ,__LINE__ -1 );
			memcpy(vanity_rmd_limit_values_B[vanity_rmd_targets][j],raw_value_B+1,20);
			
			j++;				
			values_B_size = j;
			
			target_copy[stringsize] = 'z';
			stringsize++;
		}
	}while(raw_value_length <= 25);
	
	if(values_A_size >= 1 && values_B_size >= 1)	{
		if(values_A_size != values_B_size)	{
			if(values_A_size > values_B_size)
				r = values_B_size;
			else
				r = values_A_size;
		}
		else	{
			r = values_A_size;
		}
		for(j = 0; j < r; j++)	{
			minimun_bytes =  minimum_same_bytes(vanity_rmd_limit_values_A[vanity_rmd_targets][j],vanity_rmd_limit_values_B[vanity_rmd_targets][j],20);
			if(minimun_bytes < vanity_rmd_minimun_bytes_check_length)	{
				vanity_rmd_minimun_bytes_check_length = minimun_bytes;
			}
		}
		vanity_address_targets[vanity_rmd_targets] = (char*) calloc(targetsize+1,sizeof(char));
		checkpointer((void *)vanity_address_targets[vanity_rmd_targets],__FILE__,"calloc","vanity_address_targets" ,__LINE__ -1 );
		memcpy(vanity_address_targets[vanity_rmd_targets],target,targetsize+1);	// +1 to copy the null character
		vanity_rmd_limits[vanity_rmd_targets] = r;
		vanity_rmd_total+=r;
		vanity_rmd_targets++;
	}
	else	{
		for(j = 0; j < values_A_size;j++)	{
			free(vanity_rmd_limit_values_A[vanity_rmd_targets][j]);
		}
		free(vanity_rmd_limit_values_A[vanity_rmd_targets]);
		vanity_rmd_limit_values_A[vanity_rmd_targets] = NULL;
		
		for(j = 0; j < values_B_size;j++)	{
			free(vanity_rmd_limit_values_B[vanity_rmd_targets][j]);
		}
		free(vanity_rmd_limit_values_B[vanity_rmd_targets]);
		vanity_rmd_limit_values_B[vanity_rmd_targets] = NULL;
		r = 0;
	}
	return r;
}


/*
A and B are binary o string data pointers
length the max lenght to check.

Caller must by sure that the pointer are valid and have at least length bytes readebles witout causing overflow
*/
int minimum_same_bytes(unsigned char* A,unsigned char* B, int length) {
    int minBytes = 0; // Assume initially that all bytes are the same
	if(A == NULL || B  == NULL)	{	// In case of some NULL pointer
		return 0;
	}
    for (int i = 0; i < length; i++) {
        if (A[i] != B[i]) {
            break; // Exit the loop since we found a mismatch
        }
        minBytes++; // Update the minimum number of bytes where data is the same
    }

    return minBytes;
}

void checkpointer(void *ptr,const char *file,const char *function,const  char *name,int line)	{
	if(ptr == NULL)	{
		fprintf(stderr,"[E] error in file %s, %s pointer %s on line %i\n",file,function,name,line); 
		exit(EXIT_FAILURE);
	}
}

void writekey(bool compressed,Int *key)	{
	Point publickey;
	FILE *keys;
	char *hextemp,*hexrmd,public_key_hex[132],address[50],rmdhash[20];
	memset(address,0,50);
	memset(public_key_hex,0,132);
	hextemp = key->GetBase16();
	publickey = secp->ComputePublicKey(key);
	secp->GetPublicKeyHex(compressed,publickey,public_key_hex);
	secp->GetHash160(P2PKH,compressed,publickey,(uint8_t*)rmdhash);
	hexrmd = tohex(rmdhash,20);
	rmd160toaddress_dst(rmdhash,address);

#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(write_keys, INFINITE);
#else
	pthread_mutex_lock(&write_keys);
#endif
	keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
	if(keys != NULL)	{
		fprintf(keys,"Private Key: %s\npubkey: %s\nAddress %s\nrmd160 %s\n",hextemp,public_key_hex,address,hexrmd);
		fclose(keys);
	}
	printf("\nHit! Private Key: %s\npubkey: %s\nAddress %s\nrmd160 %s\n",hextemp,public_key_hex,address,hexrmd);
	
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(write_keys);
#else
	pthread_mutex_unlock(&write_keys);
#endif
	free(hextemp);
	free(hexrmd);
}

void writekeyeth(Int *key)	{
	Point publickey;
	FILE *keys;
	char *hextemp,address[43],hash[20];
	hextemp = key->GetBase16();
	publickey = secp->ComputePublicKey(key);
	generate_binaddress_eth(publickey,(unsigned char*)hash);
	address[0] = '0';
	address[1] = 'x';
	tohex_dst(hash,20,address+2);

#if defined(_WIN64) && !defined(__CYGWIN__)
	WaitForSingleObject(write_keys, INFINITE);
#else
	pthread_mutex_lock(&write_keys);
#endif
	keys = fopen("KEYFOUNDKEYFOUND.txt","a+");
	if(keys != NULL)	{
		fprintf(keys,"Private Key: %s\naddress: %s\n",hextemp,address);
		fclose(keys);
	}
	printf("\n Hit!!!! Private Key: %s\naddress: %s\n",hextemp,address);
#if defined(_WIN64) && !defined(__CYGWIN__)
	ReleaseMutex(write_keys);
#else
	pthread_mutex_unlock(&write_keys);
#endif
	free(hextemp);
}

bool isBase58(char c) {
    // Define the base58 set
    const char base58Set[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    // Check if the character is in the base58 set
    return strchr(base58Set, c) != NULL;
}

bool isValidBase58String(char *str)	{
	int len = strlen(str);
	bool continuar = true;
	for (int i = 0; i < len && continuar; i++) {
		continuar = isBase58(str[i]);
	}
	return continuar;
}

bool processOneVanity()	{
	int i,k;
	if(vanity_rmd_targets == 0)	{
		fprintf(stderr,"[E] There aren't any vanity targets\n");
		return false;
	}

	if(!initBloomFilter(vanity_bloom, vanity_rmd_total))
		return false;
	
	for(i = 0; i < vanity_rmd_targets;i++)	{
		for(k = 0; k < vanity_rmd_limits[i]; k++)	{
			bloom_add(vanity_bloom, vanity_rmd_limit_values_A[i][k] ,vanity_rmd_minimun_bytes_check_length);
		}
	}
	return true;
}


bool readFileVanity(char *fileName)	{
	FILE *fileDescriptor;
	int i,k,len;
	char aux[100],*hextemp;

	fileDescriptor = fopen(fileName,"r");
	if(fileDescriptor == NULL)	{
		if(vanity_rmd_targets == 0)	{
			fprintf(stderr,"[E] There aren't any vanity targets\n");
			return false;
		}
	}
	else	{
		while(!feof(fileDescriptor))	{
			hextemp = fgets(aux,100,fileDescriptor);
			if(hextemp == aux)	{
				trim(aux," \t\n\r");
				len = strlen(aux);
				if(len > 0 && len < 36){
					if(isValidBase58String(aux))	{
						addvanity(aux);
					}
					else	{
						fprintf(stderr,"[E] the string \"%s\" is not valid Base58, omiting it\n",aux);
					}
				}
			}
		}
		fclose(fileDescriptor);
	}
	
	N = vanity_rmd_total;
	if(!initBloomFilter(vanity_bloom,N))
		return false;
	
	for(i = 0; i < vanity_rmd_targets ; i++)	{
		for(k = 0; k < vanity_rmd_limits[i]; k++)	{
			bloom_add(vanity_bloom, vanity_rmd_limit_values_A[i][k] ,vanity_rmd_minimun_bytes_check_length);
		}
	}
	return true;
}

bool readFileAddress(char *fileName)	{
	FILE *fileDescriptor;
	char fileBloomName[30];	/* Actually it is Bloom and Table but just to keep the variable name short*/
	uint8_t checksum[32],hexPrefix[9];
	char dataChecksum[32],bloomChecksum[32];
	size_t bytesRead;
	uint64_t dataSize;
	/*
		if the FLAGSAVEREADFILE is Set to 1 we need to the checksum and check if we have that information already saved
	*/
	if(FLAGSAVEREADFILE)	{	/* if the flag is set to REAd and SAVE the file firs we need to check it the file exist*/
		if(!sha256_file((const char*)fileName,checksum)){
			fprintf(stderr,"[E] sha256_file error line %i\n",__LINE__ - 1);
			return false;
		}
		tohex_dst((char*)checksum,4,(char*)hexPrefix); // we save the prefix (last fourt bytes) hexadecimal value
		snprintf(fileBloomName,30,"data_%s.dat",hexPrefix);
		fileDescriptor = fopen(fileBloomName,"rb");
		if(fileDescriptor != NULL)	{
			printf("[+] Reading file %s\n",fileBloomName);
		
			//read bloom checksum (expected value to be checked)
			//read bloom filter structure
			//read bloom filter data
			//calculate checksum of the current readed data
			//Compare checksums
			//read data checksum (expected value to be checked)
			//read data size
			//read data
			//compare the expected datachecksum againts the current data checksum
			//compare the expected bloom checksum againts the current bloom checksum
			

			//read bloom checksum (expected value to be checked)
			bytesRead = fread(bloomChecksum,1,32,fileDescriptor);
			if(bytesRead != 32)	{
				fprintf(stderr,"[E] Errore reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			
			//read bloom filter structure
			bytesRead = fread(&bloom,1,sizeof(struct bloom),fileDescriptor);
			if(bytesRead != sizeof(struct bloom))	{
				fprintf(stderr,"[E] Error reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			
			printf("[+] Bloom filter for %" PRIu64 " elements.\n",bloom.entries);
			
			bloom.bf = (uint8_t*) malloc(bloom.bytes);
			if(bloom.bf == NULL)	{
				fprintf(stderr,"[E] Error allocating memory, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}

			//read bloom filter data
			bytesRead = fread(bloom.bf,1,bloom.bytes,fileDescriptor);
			if(bytesRead != bloom.bytes)	{
				fprintf(stderr,"[E] Error reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			if(FLAGSKIPCHECKSUM == 0){
				
				//calculate checksum of the current readed data
				sha256((uint8_t*)bloom.bf,bloom.bytes,(uint8_t*)checksum);
				
				//Compare checksums
				/*
				if(FLAGDEBUG)	{
					hextemp = tohex((char*)checksum,32);
					printf("[D] Current Bloom checksum %s\n",hextemp);
					free(hextemp);
				}
				*/
				if(memcmp(checksum,bloomChecksum,32) != 0)	{
					fprintf(stderr,"[E] Error checksum mismatch, code line %i\n",__LINE__ - 2);
					fclose(fileDescriptor);
					return false;
				}
			}
			
			/*
			if(FLAGDEBUG) {
				hextemp = tohex((char*)bloom.bf,32);
				printf("[D] first 32 bytes of the bloom : %s\n",hextemp);
				bloom_print(&bloom);
				printf("[D] bloom.bf points to %p\n",bloom.bf);
			}
			*/
			
			bytesRead = fread(dataChecksum,1,32,fileDescriptor);
			if(bytesRead != 32)	{
				fprintf(stderr,"[E] Errore reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			
			bytesRead = fread(&dataSize,1,sizeof(uint64_t),fileDescriptor);
			if(bytesRead != sizeof(uint64_t))	{
				fprintf(stderr,"[E] Errore reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false; 
			}
			N = dataSize / sizeof(struct address_value);
	
			printf("[+] Allocating memory for %" PRIu64 " elements: %.2f MB\n",N,(double)(((double) sizeof(struct address_value)*N)/(double)1048576));
			
			addressTable = (struct address_value*) malloc(dataSize);
			if(addressTable == NULL)	{
				fprintf(stderr,"[E] Error allocating memory, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			
			bytesRead = fread(addressTable,1,dataSize,fileDescriptor);
			if(bytesRead != dataSize)	{
				fprintf(stderr,"[E] Error reading file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				return false;
			}
			if(FLAGSKIPCHECKSUM == 0)	{
					
				sha256((uint8_t*)addressTable,dataSize,(uint8_t*)checksum);
				if(memcmp(checksum,dataChecksum,32) != 0)	{
					fprintf(stderr,"[E] Error checksum mismatch, code line %i\n",__LINE__ - 2);
					fclose(fileDescriptor);
					return false;
				}
			}
			//printf("[D] bloom.bf points to %p\n",bloom.bf);
			FLAGREADEDFILE1 = 1;	/* We mark the file as readed*/
			fclose(fileDescriptor);
			MAXLENGTHADDRESS = sizeof(struct address_value);
			bf_init(&bf_filter, (uint32_t)N, 0.000001);
			for(uint64_t bi = 0; bi < N; bi++) {
				bf_add(&bf_filter, addressTable[bi].value, sizeof(struct address_value));
			}
			printf("[+] Building binary fuse filter from %" PRIu64 " cached keys... ", N);
			fflush(stdout);
			if(bf_build(&bf_filter) != 0) {
				printf("\n[!] Binary fuse failed for cached data, falling back to bloom filter\n");
				bf_filter.use_bloom_fallback = 1;
			}
			else {
				printf("done! %.2f MB\n", (double)bf_size_in_bytes(&bf_filter)/(double)1048576);
			}
		}
	}
	if(FLAGVANITY)	{
		processOneVanity();
	}
	if(!FLAGREADEDFILE1)	{
		/*
			if the data_ file doesn't exist we need read it first:
		*/
		switch(FLAGMODE)	{
			case MODE_ADDRESS:
				if(FLAGCRYPTO == CRYPTO_BTC || FLAGCRYPTO == CRYPTO_LTC || FLAGCRYPTO == CRYPTO_BTG || FLAGCRYPTO == CRYPTO_BCH || FLAGCRYPTO == CRYPTO_DOGE || FLAGCRYPTO == CRYPTO_XRP || FLAGCRYPTO == CRYPTO_ALL)	{
					return forceReadFileAddress(fileName);
				}
				if(FLAGCRYPTO == CRYPTO_ETH || FLAGCRYPTO == CRYPTO_ETC || FLAGCRYPTO == CRYPTO_ALL)	{
					return forceReadFileAddressEth(fileName);
				}
				if(FLAGCRYPTO == CRYPTO_TROOT)	{
					if(!initBloomFilter(&troot_bloom, 1024)) return false;
					bf_init(&troot_bf_filter, 1024, 0.000001);
					return forceReadFileTroot(fileName);
				}
			break;
			case MODE_MINIKEYS:
			case MODE_RMD160:
			case MODE_MNEMONIC:
			case MODE_POETRY:
			case MODE_BRAINWALLET:
			case MODE_PUB2ADDR:
				return forceReadFileAddress(fileName);
			break;
			case MODE_XPOINT:
				return forceReadFileXPoint(fileName);
			break;
			default:
				return false;
			break;
		}
	}
	return true;
}

bool forceReadFileAddress(char *fileName)	{
	/* Here we read the original file as usual */
	FILE *fileDescriptor;
	bool validAddress;
	uint64_t numberItems,i;
	size_t r,raw_value_length;
	uint8_t rawvalue[50];
	char aux[100],*hextemp;
	fileDescriptor = fopen(fileName,"r");	
	if(fileDescriptor == NULL)	{
		fprintf(stderr,"[E] Error opening the file %s, line %i\n",fileName,__LINE__ - 2);
		return false;
	}

	/*Count lines in the file*/
	numberItems = 0;
	while(!feof(fileDescriptor))	{
		hextemp = fgets(aux,100,fileDescriptor);
		trim(aux," \t\n\r");
		if(hextemp == aux)	{			
			r = strlen(aux);
			if(r > 20)	{ 
				numberItems++;
			}
		}
	}
	fseek(fileDescriptor,0,SEEK_SET);
	MAXLENGTHADDRESS = 20;		/*20 bytes beacuase we only need the data in binary*/
	
	printf("[+] Allocating memory for %" PRIu64 " elements: %.2f MB\n",numberItems,(double)(((double) sizeof(struct address_value)*numberItems)/(double)1048576));
	addressTable = (struct address_value*) malloc(sizeof(struct address_value)*numberItems);
	checkpointer((void *)addressTable,__FILE__,"malloc","addressTable" ,__LINE__ -1 );
		
	if(!initBloomFilter(&bloom,numberItems))
		return false;

	i = 0;
	while(i < numberItems)	{
		validAddress = false;
		memset(aux,0,100);
		memset(addressTable[i].value,0,sizeof(struct address_value));
		hextemp = fgets(aux,100,fileDescriptor);
		trim(aux," \t\n\r");			
		r = strlen(aux);
		if(r > 0 && r <= 40)	{
				if(r<40 && isValidBase58String(aux))	{	//Address
				raw_value_length = 25;
				b58tobin(rawvalue,&raw_value_length,aux,r);
				if(raw_value_length == 25)	{
					bloom_add(&bloom, rawvalue+1 ,sizeof(struct address_value));
					bf_add(&bf_filter, rawvalue+1 ,sizeof(struct address_value));
					memcpy(addressTable[i].value,rawvalue+1,sizeof(struct address_value));
					i++;
					validAddress = true;
				}
			}
			if(r == 40 && isValidHex(aux))	{	//RMD
				hexs2bin(aux,rawvalue);
				bloom_add(&bloom, rawvalue ,sizeof(struct address_value));
				bf_add(&bf_filter, rawvalue ,sizeof(struct address_value));
				memcpy(addressTable[i].value,rawvalue,sizeof(struct address_value));
				i++;
				validAddress = true;
			}
		}
		if(!validAddress)	{
			fprintf(stderr,"[I] Ommiting invalid line %s\n",aux);
			numberItems--;
		}
	}
	N = numberItems;
	printf("[+] Building binary fuse filter from %" PRIu64 " keys... ", N);
	fflush(stdout);
	if(bf_build(&bf_filter) != 0) {
		printf("\n[!] Binary fuse failed, falling back to bloom filter\n");
		bf_filter.use_bloom_fallback = 1;
		bloom_add(&bloom, rawvalue ,sizeof(struct address_value));
	}
	else {
		printf("done! %.2f MB\n", (double)bf_size_in_bytes(&bf_filter)/(double)1048576);
	}
	return true;
}

bool forceReadFileAddressEth(char *fileName)	{
	/* Here we read the original file as usual */
	FILE *fileDescriptor;
	bool validAddress;
	uint64_t numberItems,i;
	size_t r;
	uint8_t rawvalue[50];
	char aux[100],*hextemp;
	fileDescriptor = fopen(fileName,"r");	
	if(fileDescriptor == NULL)	{
		fprintf(stderr,"[E] Error opening the file %s, line %i\n",fileName,__LINE__ - 2);
		return false;
	}
	/*Count lines in the file*/
	numberItems = 0;
	while(!feof(fileDescriptor))	{
		hextemp = fgets(aux,100,fileDescriptor);
		trim(aux," \t\n\r");
		if(hextemp == aux)	{			
			r = strlen(aux);
			if(r >= 40)	{ 
				numberItems++;
			}
		}
	}
	fseek(fileDescriptor,0,SEEK_SET);

	MAXLENGTHADDRESS = 20;		/*20 bytes beacuase we only need the data in binary*/
	N = numberItems;
	
	printf("[+] Allocating memory for %" PRIu64 " elements: %.2f MB\n",numberItems,(double)(((double) sizeof(struct address_value)*numberItems)/(double)1048576));
	addressTable = (struct address_value*) malloc(sizeof(struct address_value)*numberItems);
	checkpointer((void *)addressTable,__FILE__,"malloc","addressTable" ,__LINE__ -1 );
	
	
	if(!initBloomFilter(&bloom,N))
		return false;
	
	i = 0;
	while(i < numberItems)	{
		validAddress = false;
		memset(aux,0,100);
		memset(addressTable[i].value,0,sizeof(struct address_value));
		hextemp = fgets(aux,100,fileDescriptor);
		trim(aux," \t\n\r");			
		r = strlen(aux);
		if(r >= 40 && r <= 42){
			switch(r)		{
			case 40:
				if(isValidHex(aux)){
					hexs2bin(aux,rawvalue);
					bloom_add(&bloom, rawvalue ,sizeof(struct address_value));
					bf_add(&bf_filter, rawvalue ,sizeof(struct address_value));
					memcpy(addressTable[i].value,rawvalue,sizeof(struct address_value));
					i++;
					validAddress = true;
				}
			break;
			case 42:
				if(isValidHex(aux+2)){
					hexs2bin(aux+2,rawvalue);
					bloom_add(&bloom, rawvalue ,sizeof(struct address_value));
					bf_add(&bf_filter, rawvalue ,sizeof(struct address_value));
					memcpy(addressTable[i].value,rawvalue,sizeof(struct address_value));
					i++;
					validAddress = true;
				}
					break;
			}
		}
		if(!validAddress)	{
			fprintf(stderr,"[I] Ommiting invalid line %s\n",aux);
			numberItems--;
		}
	}
	
	fclose(fileDescriptor);
	printf("[+] Building binary fuse filter from %" PRIu64 " keys... ", N);
	fflush(stdout);
	if(bf_build(&bf_filter) != 0) {
		printf("\n[!] Binary fuse failed, falling back to bloom filter\n");
		bf_filter.use_bloom_fallback = 1;
	}
	else {
		printf("done! %.2f MB\n", (double)bf_size_in_bytes(&bf_filter)/(double)1048576);
	}
	return true;
}



bool forceReadFileXPoint(char *fileName)	{
	/* Here we read the original file as usual */
	FILE *fileDescriptor;
	uint64_t numberItems,i;
	size_t r,lenaux;
	uint8_t rawvalue[100];
	char aux[1000],*hextemp;
	Tokenizer tokenizer_xpoint;	//tokenizer
	fileDescriptor = fopen(fileName,"r");	
	if(fileDescriptor == NULL)	{
		fprintf(stderr,"[E] Error opening the file %s, line %i\n",fileName,__LINE__ - 2);
		return false;
	}
	/*Count lines in the file*/
	numberItems = 0;
	while(!feof(fileDescriptor))	{
		hextemp = fgets(aux,1000,fileDescriptor);
		trim(aux," \t\n\r");
		if(hextemp == aux)	{			
			r = strlen(aux);
			if(r >= 40)	{ 
				numberItems++;
			}
		}
	}
	fseek(fileDescriptor,0,SEEK_SET);

	MAXLENGTHADDRESS = 20;		/*20 bytes beacuase we only need the data in binary*/
	
	printf("[+] Allocating memory for %" PRIu64 " elements: %.2f MB\n",numberItems,(double)(((double) sizeof(struct address_value)*numberItems)/(double)1048576));
	addressTable = (struct address_value*) malloc(sizeof(struct address_value)*numberItems);
	checkpointer((void *)addressTable,__FILE__,"malloc","addressTable" ,__LINE__ - 1);
	
	N = numberItems;
	
	if(!initBloomFilter(&bloom,N))
		return false;
	
	i= 0;
	while(i < N)	{
		memset(aux,0,1000);
		hextemp = fgets(aux,1000,fileDescriptor);
		memset((void *)&addressTable[i],0,sizeof(struct address_value));
		if(hextemp == aux)	{
			trim(aux," \t\n\r");
			stringtokenizer(aux,&tokenizer_xpoint);
			hextemp = nextToken(&tokenizer_xpoint);
			lenaux = strlen(hextemp);
			if(isValidHex(hextemp)) {
				switch(lenaux)	{
					case 64:	/*X value*/
						r = hexs2bin(aux,(uint8_t*) rawvalue);
						if(r)	{
							memcpy(addressTable[i].value,rawvalue,20);
							bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
							bf_add(&bf_filter,rawvalue,MAXLENGTHADDRESS);
						}
						else	{
							fprintf(stderr,"[E] error hexs2bin\n");
						}
					break;
					case 66:	/*Compress publickey*/
						r = hexs2bin(aux+2, (uint8_t*)rawvalue);
						if(r)	{
							memcpy(addressTable[i].value,rawvalue,20);
							bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
							bf_add(&bf_filter,rawvalue,MAXLENGTHADDRESS);
						}
						else	{
							fprintf(stderr,"[E] error hexs2bin\n");
						}
					break;
					case 130:	/* Uncompress publickey length*/
						r = hexs2bin(aux, (uint8_t*) rawvalue);
						if(r)	{
								memcpy(addressTable[i].value,rawvalue+2,20);
								bloom_add(&bloom,rawvalue,MAXLENGTHADDRESS);
								bf_add(&bf_filter,rawvalue,MAXLENGTHADDRESS);
						}
						else	{
							fprintf(stderr,"[E] error hexs2bin\n");
						}
					break;
					default:
						fprintf(stderr,"[E] Omiting line unknow length size %li: %s\n",lenaux,aux);
					break;
				}
			}
			else	{
				fprintf(stderr,"[E] Ignoring invalid hexvalue %s\n",aux);
			}
			freetokenizer(&tokenizer_xpoint);
		}
		else	{
			fprintf(stderr,"[E] Omiting line : %s\n",aux);
			N--;
		}
		i++;
	}
	fclose(fileDescriptor);
	printf("[+] Building binary fuse filter from %" PRIu64 " keys... ", N);
	fflush(stdout);
	if(bf_build(&bf_filter) != 0) {
		printf("\n[!] Binary fuse failed, falling back to bloom filter\n");
		bf_filter.use_bloom_fallback = 1;
	}
	else {
		printf("done! %.2f MB\n", (double)bf_size_in_bytes(&bf_filter)/(double)1048576);
	}
	return true;
}


/*
	I write this as a function because i have the same segment of code in 3 different functions
*/

int address_check(const void *buffer, int len) {
	int r = bf_check(&bf_filter, buffer, len);
	if(r == -1 && bf_filter.use_bloom_fallback) {
		r = bloom_check(&bloom, buffer, len);
	}
	return r;
}

bool initBloomFilter(struct bloom *bloom_arg,uint64_t items_bloom)	{
	bool r = true;
	printf("[+] Binary fuse filter for %" PRIu64 " elements.\n",items_bloom);
	uint32_t bf_count = (items_bloom > 10000) ? (uint32_t)(FLAGBLOOMMULTIPLIER * items_bloom) : 10000;
	bf_init(&bf_filter, bf_count, 0.000001);
	if(bf_filter.keys == NULL)	{
		fprintf(stderr,"[E] error binary fuse alloc for %" PRIu64 " elements.\n",items_bloom);
		r = false;
	}
	else	{
		size_t mem_est = bf_count * sizeof(uint64_t);
		printf("[+] Collecting keys: %.2f MB temp memory\n",(double)mem_est/(double)1048576);
	}
	if(items_bloom <= 10000)	{
		if(bloom_init2(bloom_arg,10000,0.000001) == 1){
			fprintf(stderr,"[E] error bloom_init for 10000 elements.\n");
			r = false;
		}
	}
	else	{
		if(bloom_init2(bloom_arg,FLAGBLOOMMULTIPLIER*items_bloom,0.000001)	== 1){
			fprintf(stderr,"[E] error bloom_init for %" PRIu64 " elements.\n",items_bloom);
			r = false;
		}
	}
	return r;
}

void writeFileIfNeeded(const char *fileName)	{
	//printf("[D] FLAGSAVEREADFILE %i, FLAGREADEDFILE1 %i\n",FLAGSAVEREADFILE,FLAGREADEDFILE1);
	if(FLAGSAVEREADFILE && !FLAGREADEDFILE1)	{
		FILE *fileDescriptor;
		char fileBloomName[30];
		uint8_t checksum[32],hexPrefix[9];
		char dataChecksum[32],bloomChecksum[32];
		size_t bytesWrite;
		uint64_t dataSize;
		if(!sha256_file((const char*)fileName,checksum)){
			fprintf(stderr,"[E] sha256_file error line %i\n",__LINE__ - 1);
			exit(EXIT_FAILURE);
		}
		tohex_dst((char*)checksum,4,(char*)hexPrefix); // we save the prefix (last fourt bytes) hexadecimal value
		snprintf(fileBloomName,30,"data_%s.dat",hexPrefix);
		fileDescriptor = fopen(fileBloomName,"wb");
		dataSize = N * (sizeof(struct address_value));
		printf("[D] size data %li\n",dataSize);
		if(fileDescriptor != NULL)	{
			printf("[+] Writing file %s ",fileBloomName);
			

			//calculate bloom checksum
			//write bloom checksum (expected value to be checked)
			//write bloom filter structure
			//write bloom filter data


			//calculate dataChecksum
			//write data checksum (expected value to be checked)
			//write data size
			//write data
			
			
			

			sha256((uint8_t*)bloom.bf,bloom.bytes,(uint8_t*)bloomChecksum);
			printf(".");
			bytesWrite = fwrite(bloomChecksum,1,32,fileDescriptor);
			if(bytesWrite != 32)	{
				fprintf(stderr,"[E] Errore writing file, code line %i\n",__LINE__ - 2);
				exit(EXIT_FAILURE);
			}
			printf(".");
			
			bytesWrite = fwrite(&bloom,1,sizeof(struct bloom),fileDescriptor);
			if(bytesWrite != sizeof(struct bloom))	{
				fprintf(stderr,"[E] Error writing file, code line %i\n",__LINE__ - 2);
				exit(EXIT_FAILURE);
			}
			printf(".");
			
			bytesWrite = fwrite(bloom.bf,1,bloom.bytes,fileDescriptor);
			if(bytesWrite != bloom.bytes)	{
				fprintf(stderr,"[E] Error writing file, code line %i\n",__LINE__ - 2);
				fclose(fileDescriptor);
				exit(EXIT_FAILURE);
			}
			printf(".");
			
			/*
			if(FLAGDEBUG)	{
				hextemp = tohex((char*)bloom.bf,32);
				printf("\n[D] first 32 bytes bloom : %s\n",hextemp);
				bloom_print(&bloom);
				free(hextemp);
			}
			*/

			
			
			sha256((uint8_t*)addressTable,dataSize,(uint8_t*)dataChecksum);
			printf(".");

			bytesWrite = fwrite(dataChecksum,1,32,fileDescriptor);
			if(bytesWrite != 32)	{
				fprintf(stderr,"[E] Errore writing file, code line %i\n",__LINE__ - 2);
				exit(EXIT_FAILURE);
			}
			printf(".");	
			
			bytesWrite = fwrite(&dataSize,1,sizeof(uint64_t),fileDescriptor);
			if(bytesWrite != sizeof(uint64_t))	{
				fprintf(stderr,"[E] Errore writing file, code line %i\n",__LINE__ - 2);
				exit(EXIT_FAILURE);
			}
			printf(".");
			
			bytesWrite = fwrite(addressTable,1,dataSize,fileDescriptor);
			if(bytesWrite != dataSize)	{
				fprintf(stderr,"[E] Error writing file, code line %i\n",__LINE__ - 2);
				exit(EXIT_FAILURE);
			}
			printf(".");
			
			FLAGREADEDFILE1 = 1;	
			fclose(fileDescriptor);		
			printf("\n");
		}
	}
}

void calcualteindex(int i,Int *key)	{
	if(i == 0)	{
		key->Set(&BSGS_M3);
	}
	else	{
		key->SetInt32(i);
		key->Mult(&BSGS_M3_double);
		key->Add(&BSGS_M3);
	}
}
