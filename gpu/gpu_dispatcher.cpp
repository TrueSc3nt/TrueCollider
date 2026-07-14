/*
 * GPU dispatcher for TrueCollider.
 *
 * CUDA path: GPU does secp256k1 pubkey derivation; host (MSVC) does hash160 + bloom.
 */

#include "gpu_dispatcher.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <inttypes.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(ENABLE_CUDA)
#include <cuda_runtime.h>
#endif

#include "../hash/sha256.h"
#include "../hash/ripemd160.h"
#include "../bloom/bloom.h"
#include "../sha3/sha3.h"

#ifndef ENABLE_CUDA
extern "C" int tcuda_device_count(void) { return 0; }
extern "C" int tcuda_hello(void) { return 0; }
extern "C" int tcuda_hash160_33_selftest(void) { return 0; }
extern "C" int tcuda_hash160_33_batch(const uint8_t*, int, uint8_t*) { return 0; }
extern "C" int tcuda_secp_selftest(void) { return 0; }
extern "C" int tcuda_secp_device_filter(void) { return 0; }
extern "C" int tcuda_secp_search_init(const uint8_t*, uint64_t, uint64_t, uint8_t) { return 0; }
extern "C" void tcuda_secp_search_free(void) {}
extern "C" int tcuda_secp_search_batch(const uint8_t*, int, int, uint32_t*, int, uint32_t*) { return 0; }
extern "C" int tcuda_secp_pubkey_batch(const uint8_t*, int, int, uint8_t*) { return -1; }
extern "C" int tcuda_bsgs_grp_init(const uint8_t*, int, const uint8_t*) { return 0; }
extern "C" void tcuda_bsgs_grp_free(void) {}
extern "C" int tcuda_bsgs_grp_run(uint8_t*, int, uint8_t*) { return 0; }
extern "C" int tcuda_bsgs_grp_ready(void) { return 0; }
extern "C" int tcuda_memory_info(uint64_t *free_bytes, uint64_t *total_bytes) {
    (void)free_bytes; (void)total_bytes; return 0;
}
extern "C" uint32_t tcuda_apply_memory_budget(uint64_t budget_bytes) {
    (void)budget_bytes; return 0;
}
#else
extern "C" int tcuda_device_count(void);
extern "C" int tcuda_hello(void);
extern "C" int tcuda_hash160_33_selftest(void);
extern "C" int tcuda_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out);
extern "C" int tcuda_secp_selftest(void);
extern "C" int tcuda_secp_device_filter(void);
extern "C" int tcuda_secp_search_init(const uint8_t *bloom_bf, uint64_t bloom_bits,
                                      uint64_t bloom_bytes, uint8_t bloom_hashes);
extern "C" void tcuda_secp_search_free(void);
extern "C" int tcuda_secp_search_batch(const uint8_t *privkeys, int count, int compressed,
                                       uint32_t *match_indices, int max_matches,
                                       uint32_t *out_hit_count);
extern "C" int tcuda_secp_pubkey_batch(const uint8_t *privkeys, int count, int compressed,
                                       uint8_t *out_pubs65);
extern "C" int tcuda_bsgs_grp_init(const uint8_t *gsn_xy64, int half, const uint8_t *twogsn_xy64);
extern "C" void tcuda_bsgs_grp_free(void);
extern "C" int tcuda_bsgs_grp_run(uint8_t *start_xy64, int n_cycles, uint8_t *out_x32);
extern "C" int tcuda_bsgs_grp_ready(void);
extern "C" int tcuda_memory_info(uint64_t *free_bytes, uint64_t *total_bytes);
extern "C" uint32_t tcuda_apply_memory_budget(uint64_t budget_bytes);
extern "C" int tcuda_ed25519_pubkey_batch(const uint8_t *seeds32, int count, uint8_t *pubs32);
extern "C" int tcuda_ed25519_selftest(void);
#endif

#ifndef ENABLE_CUDA
extern "C" int tcuda_ed25519_pubkey_batch(const uint8_t*, int, uint8_t*) { return 0; }
extern "C" int tcuda_ed25519_selftest(void) { return 0; }
extern "C" int tcuda_ed25519_sha512_clamp_batch(const uint8_t*, int, uint8_t*) { return 0; }
#endif

#ifndef ENABLE_OPENCL
extern "C" int topencl_device_count(void) { return 0; }
extern "C" int topencl_hello(void) { return 0; }
extern "C" int topencl_hash160_33_selftest(void) { return 0; }
extern "C" int topencl_hash160_33_batch(const uint8_t*, int, uint8_t*) { return 0; }
#else
extern "C" int topencl_device_count(void);
extern "C" int topencl_hello(void);
extern "C" int topencl_hash160_33_selftest(void);
extern "C" int topencl_hash160_33_batch(const uint8_t *keys33, int count, uint8_t *out20);
#endif

struct GpuDispatcher {
    struct BackendConfig cfg;
    int initialized;
    int device_count;
    int secp_ready;
    int ed25519_ready;
    int bloom_loaded;
    struct bloom host_bloom;
#ifdef _WIN32
    CRITICAL_SECTION gpu_lock;
    int gpu_lock_ready;
#else
    pthread_mutex_t gpu_lock;
#endif
};

static void host_hash160_pub(const uint8_t *pub, int compressed, uint8_t out20[20]) {
    uint8_t sha[32];
    if (compressed)
        sha256_33((uint8_t *)pub, sha);
    else
        sha256_65((uint8_t *)pub, sha);
    ripemd160_32(sha, out20);
}

/* ETH: keccak256(uncompressed X||Y) → last 20 bytes. pub must be 0x04||X||Y. */
static void host_eth_addr_pub(const uint8_t *pub65, uint8_t out20[20]) {
    uint8_t dig[32];
    SHA3_256_CTX ctx;
    SHA3_256_Init(&ctx);
    SHA3_256_Update(&ctx, pub65 + 1, 64);
    KECCAK_256_Final(dig, &ctx);
    memcpy(out20, dig + 12, 20);
}

struct GpuDispatcher* gpu_dispatcher_create(void) {
    struct GpuDispatcher* d = (struct GpuDispatcher*)calloc(1, sizeof(struct GpuDispatcher));
    if (!d) return NULL;
#ifdef _WIN32
    InitializeCriticalSection(&d->gpu_lock);
    d->gpu_lock_ready = 1;
#else
    pthread_mutex_init(&d->gpu_lock, NULL);
#endif
    return d;
}

void gpu_dispatcher_destroy(struct GpuDispatcher* disp) {
    if (!disp) return;
#if defined(ENABLE_CUDA)
    tcuda_secp_search_free();
#endif
#ifdef _WIN32
    if (disp->gpu_lock_ready) DeleteCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_destroy(&disp->gpu_lock);
#endif
    free(disp);
}

int gpu_dispatcher_init(struct GpuDispatcher* disp, const struct BackendConfig* cfg) {
    if (!disp || !cfg) return 0;
    setvbuf(stderr, nullptr, _IONBF, 0);
    setvbuf(stdout, nullptr, _IONBF, 0);
    disp->cfg = *cfg;
    disp->initialized = 0;
    disp->device_count = 0;
    disp->secp_ready = 0;
    disp->ed25519_ready = 0;
    disp->bloom_loaded = 0;
    memset(&disp->host_bloom, 0, sizeof(disp->host_bloom));

    if (cfg->gpu_backend == GPU_BACKEND_CUDA) {
        disp->device_count = tcuda_device_count();
        if (disp->device_count <= 0) {
            std::fprintf(stderr, "[E] CUDA backend requested but no CUDA devices were found.\n");
            return 0;
        }
        tcuda_hello();
        int hash_ok = tcuda_hash160_33_selftest();
        if (!hash_ok) {
            std::fprintf(stderr, "[W] CUDA hash160 self-test failed; host-EC GPU-hash offload disabled.\n");
        }
        if (!tcuda_secp_selftest()) {
            std::fprintf(stderr, "[W] CUDA secp256k1 self-test failed; full GPU search disabled.\n");
            disp->secp_ready = 0;
            if (!hash_ok) {
                std::fprintf(stderr, "[E] No working CUDA path (hash160 and secp both failed).\n");
                return 0;
            }
        } else {
            if (hash_ok)
                std::fprintf(stderr, "[+] CUDA secp256k1 path ready (GPU EC + device hash160/bloom).\n");
            else
                std::fprintf(stderr, "[+] CUDA secp256k1 path ready (GPU EC + host hash/bloom).\n");
            disp->secp_ready = 1;
        }

        if (tcuda_ed25519_selftest()) {
            disp->ed25519_ready = 1;
        } else {
            std::fprintf(stderr, "[W] CUDA ed25519 self-test failed; Solana falls back to CPU.\n");
            disp->ed25519_ready = 0;
        }

        /* Memory budget: -M MB|auto; default auto under CUDA. */
        {
            uint64_t budget = 0;
            if (!cfg->memory_auto && cfg->memory_budget_bytes > 0)
                budget = cfg->memory_budget_bytes;
            uint32_t rec = tcuda_apply_memory_budget(budget);
            if (rec > 0) {
                if (cfg->gpu_batch_user_set) {
                    uint32_t want = cfg->gpu_batch_size;
                    if (want > rec) {
                        std::fprintf(stderr, "[W] -G %u exceeds memory budget; clamping to %u.\n",
                                     want, rec);
                        want = rec;
                    }
                    if (want < 1) want = 1;
                    disp->cfg.gpu_batch_size = want;
                } else {
                    disp->cfg.gpu_batch_size = rec;
                }
            }
        }

        std::fprintf(stderr, "[+] CUDA backend selected (%d device%s), effective batch %u keys.\n",
                     disp->device_count, disp->device_count > 1 ? "s" : "",
                     disp->cfg.gpu_batch_size);
        /* Keep global config in sync for thread loops that read g_backend_config. */
        g_backend_config.gpu_batch_size = disp->cfg.gpu_batch_size;
        disp->initialized = 1;
        return 1;
    }

    if (cfg->gpu_backend == GPU_BACKEND_OPENCL) {
        disp->device_count = topencl_device_count();
        if (disp->device_count <= 0) {
            std::fprintf(stderr, "[E] OpenCL backend requested but no GPU devices were found.\n");
            return 0;
        }
        topencl_hello();
        if (!topencl_hash160_33_selftest()) {
            std::fprintf(stderr, "[W] OpenCL hash160 self-test failed; GPU hash offload disabled.\n");
            return 0;
        }
        std::fprintf(stderr, "[+] OpenCL hash160 path ready (NVIDIA/AMD/Intel GPUs via OpenCL).\n");
        std::fprintf(stderr, "[+] OpenCL backend selected (%d device%s). "
                     "secp EC stays on CPU; use -U cuda for GPU EC on NVIDIA.\n",
                     disp->device_count, disp->device_count > 1 ? "s" : "");
        disp->initialized = 1;
        disp->secp_ready = 0;
        return 1;
    }

    return 0;
}

int gpu_dispatcher_available(const struct GpuDispatcher* disp) {
    return disp && disp->initialized;
}

int gpu_dispatcher_secp_ready(const struct GpuDispatcher* disp) {
    return disp && disp->secp_ready;
}

int gpu_dispatcher_ed25519_ready(const struct GpuDispatcher* disp) {
    return disp && disp->ed25519_ready;
}

int gpu_dispatcher_supports_mode(const struct GpuDispatcher* disp, int mode) {
    if (!disp || !disp->initialized) return 0;
    /* CUDA secp EC assists all secp-based search modes.
       MODE_*: xpoint=0 address=1 bsgs=2 rmd160=3 minikeys=5 vanity=6
       mnemonic=7 poetry=8 brainwallet=9 pubkey2addr=10 kangaroo=11.
       address + -c sol uses ed25519 CUDA path when ready. */
    switch (mode) {
        case 0: /* xpoint */
        case 1: /* address (BTC secp or SOL ed25519) */
        case 2: /* bsgs baby + giant ComputePublicKey assist */
        case 3: /* rmd160 */
        case 5: /* minikeys */
        case 6: /* vanity */
        case 7: /* mnemonic */
        case 8: /* poetry */
        case 9: /* brainwallet */
        case 10: /* pubkey2addr */
            return disp->secp_ready || disp->ed25519_ready || disp->initialized;
        default:
            return 0;
    }
}

int gpu_dispatcher_hash160_33(struct GpuDispatcher* disp,
                              const uint8_t* keys33,
                              uint32_t count,
                              uint8_t* out20) {
    if (!disp || !disp->initialized || !keys33 || !out20 || count == 0) return 0;
#if defined(ENABLE_CUDA)
    if (disp->cfg.gpu_backend == GPU_BACKEND_CUDA)
        return tcuda_hash160_33_batch(keys33, (int)count, out20);
#endif
#if defined(ENABLE_OPENCL)
    if (disp->cfg.gpu_backend == GPU_BACKEND_OPENCL)
        return topencl_hash160_33_batch(keys33, (int)count, out20);
#endif
    return 0;
}

int gpu_dispatcher_load_bloom(struct GpuDispatcher* disp,
                              const uint8_t* bf, uint64_t bits, uint64_t bytes, uint8_t hashes) {
    if (!disp || !disp->initialized || !bf || bytes == 0) return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA) {
        std::fprintf(stderr, "[W] Device bloom upload is currently CUDA-only.\n");
        return 0;
    }
    memset(&disp->host_bloom, 0, sizeof(disp->host_bloom));
    disp->host_bloom.bf = (uint8_t *)bf;
    disp->host_bloom.bits = bits;
    disp->host_bloom.bytes = bytes;
    disp->host_bloom.hashes = hashes;
    disp->host_bloom.ready = 1;

    if (!tcuda_secp_search_init(bf, bits, bytes, hashes)) {
        std::fprintf(stderr, "[E] Failed to init GPU secp search state.\n");
        disp->bloom_loaded = 0;
        return 0;
    }
    disp->bloom_loaded = 1;
    if (tcuda_secp_device_filter())
        std::fprintf(stderr, "[+] Bloom ready for GPU-EC path (%" PRIu64 " bytes, %u hashes, device filter).\n",
                     bytes, (unsigned)hashes);
    else
        std::fprintf(stderr, "[+] Bloom ready for GPU-EC path (%" PRIu64 " bytes, %u hashes, host filter).\n",
                     bytes, (unsigned)hashes);
    return 1;
}

uint32_t gpu_dispatcher_search_privkeys(struct GpuDispatcher* disp,
                                        const uint8_t* privkeys,
                                        uint32_t count,
                                        int compressed,
                                        int encode,
                                        uint32_t* match_indices,
                                        uint32_t max_matches) {
    if (!disp || !disp->initialized || !disp->secp_ready || !disp->bloom_loaded)
        return 0;
    if (!privkeys || count == 0 || !disp->host_bloom.ready)
        return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA)
        return 0;
    if (encode == GPU_ENCODE_ETH && compressed)
        return 0; /* ETH needs uncompressed X||Y */

#ifdef _WIN32
    EnterCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_lock(&disp->gpu_lock);
#endif

    if (encode == GPU_ENCODE_HASH160 && tcuda_secp_device_filter()) {
        uint32_t hits = 0;
        int src = tcuda_secp_search_batch(privkeys, (int)count, compressed,
                                          match_indices, (int)max_matches, &hits);
        if (src >= 0) {
#ifdef _WIN32
            LeaveCriticalSection(&disp->gpu_lock);
#else
            pthread_mutex_unlock(&disp->gpu_lock);
#endif
            return hits;
        }
        /* src == -2 or error: fall through to host hash/bloom */
    }

    /* Stack buffer for the stable single-key path; VirtualAlloc grow-only for larger. */
    uint8_t stack_pubs[65];
    uint8_t *pubs = stack_pubs;
    static uint8_t *heap_pubs = NULL;
    static size_t heap_cap = 0;

    if (count != 1) {
        size_t need = (size_t)count * 65;
        if (need > heap_cap) {
#ifdef _WIN32
            uint8_t *nbuf = (uint8_t *)VirtualAlloc(NULL, need, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (nbuf && heap_pubs) VirtualFree(heap_pubs, 0, MEM_RELEASE);
#else
            uint8_t *nbuf = (uint8_t *)realloc(heap_pubs, need);
#endif
            if (!nbuf) {
#ifdef _WIN32
                LeaveCriticalSection(&disp->gpu_lock);
#else
                pthread_mutex_unlock(&disp->gpu_lock);
#endif
                return 0;
            }
            heap_pubs = nbuf;
            heap_cap = need;
        }
        pubs = heap_pubs;
    }

    int rc = tcuda_secp_pubkey_batch(privkeys, (int)count, compressed, pubs);
    uint32_t hits = 0, stored = 0;
    if (rc == 0) {
        for (uint32_t i = 0; i < count; i++) {
            const uint8_t *pub = pubs + (size_t)i * 65;
            if (pub[0] == 0) continue;
            uint8_t h20[20];
            if (encode == GPU_ENCODE_ETH) {
                if (pub[0] != 0x04) continue;
                host_eth_addr_pub(pub, h20);
            } else {
                host_hash160_pub(pub, compressed, h20);
            }
            if (bloom_check(&disp->host_bloom, h20, 20) != 1)
                continue;
            hits++;
            if (match_indices && stored < max_matches)
                match_indices[stored++] = i;
        }
    }
#ifdef _WIN32
    LeaveCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_unlock(&disp->gpu_lock);
#endif
    return (rc == 0) ? hits : 0;
}

int gpu_dispatcher_pubkey_batch(struct GpuDispatcher* disp,
                                const uint8_t* privkeys,
                                uint32_t count,
                                int compressed,
                                uint8_t* out_pubs65) {
    if (!disp || !disp->initialized || !disp->secp_ready)
        return 0;
    if (!privkeys || !out_pubs65 || count == 0)
        return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA)
        return 0;
#ifdef _WIN32
    EnterCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_lock(&disp->gpu_lock);
#endif
    int rc = tcuda_secp_pubkey_batch(privkeys, (int)count, compressed, out_pubs65);
#ifdef _WIN32
    LeaveCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_unlock(&disp->gpu_lock);
#endif
    return rc == 0 ? 1 : 0;
}

int gpu_dispatcher_ed25519_pubkey_batch(struct GpuDispatcher* disp,
                                        const uint8_t* seeds32,
                                        uint32_t count,
                                        uint8_t* out_pubs32) {
    if (!disp || !disp->initialized || !disp->ed25519_ready)
        return 0;
    if (!seeds32 || !out_pubs32 || count == 0)
        return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA)
        return 0;
#ifdef _WIN32
    EnterCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_lock(&disp->gpu_lock);
#endif
    int ok = tcuda_ed25519_pubkey_batch(seeds32, (int)count, out_pubs32);
#ifdef _WIN32
    LeaveCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_unlock(&disp->gpu_lock);
#endif
    return ok ? 1 : 0;
}

int gpu_dispatcher_bsgs_grp_init(struct GpuDispatcher* disp,
                                 const uint8_t* gsn_xy64, int half,
                                 const uint8_t* twogsn_xy64) {
    if (!disp || !disp->initialized || !disp->secp_ready) return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA) return 0;
#ifdef _WIN32
    EnterCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_lock(&disp->gpu_lock);
#endif
    int ok = tcuda_bsgs_grp_init(gsn_xy64, half, twogsn_xy64);
#ifdef _WIN32
    LeaveCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_unlock(&disp->gpu_lock);
#endif
    return ok;
}

int gpu_dispatcher_bsgs_grp_ready(const struct GpuDispatcher* disp) {
    if (!disp || !disp->initialized || !disp->secp_ready) return 0;
    if (disp->cfg.gpu_backend != GPU_BACKEND_CUDA) return 0;
    return tcuda_bsgs_grp_ready();
}

int gpu_dispatcher_bsgs_grp_run(struct GpuDispatcher* disp,
                                uint8_t* start_xy64, int n_cycles, uint8_t* out_x32) {
    if (!disp || !disp->initialized || !start_xy64 || !out_x32 || n_cycles <= 0)
        return 0;
    if (!tcuda_bsgs_grp_ready()) return 0;
#ifdef _WIN32
    EnterCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_lock(&disp->gpu_lock);
#endif
    int ok = tcuda_bsgs_grp_run(start_xy64, n_cycles, out_x32);
#ifdef _WIN32
    LeaveCriticalSection(&disp->gpu_lock);
#else
    pthread_mutex_unlock(&disp->gpu_lock);
#endif
    return ok;
}

uint32_t gpu_dispatcher_search_address(struct GpuDispatcher* disp,
                                       const uint8_t* keys33,
                                       uint32_t count,
                                       uint32_t* matches) {
    (void)disp; (void)keys33; (void)count; (void)matches;
    return 0;
}

uint32_t gpu_dispatcher_search_rmd160(struct GpuDispatcher* disp,
                                      const uint8_t* keys33,
                                      uint32_t count,
                                      uint32_t* matches) {
    return gpu_dispatcher_search_address(disp, keys33, count, matches);
}

const char* gpu_dispatcher_name(const struct GpuDispatcher* disp) {
    if (!disp || !disp->initialized) return "none";
    if (disp->cfg.gpu_backend == GPU_BACKEND_CUDA) return "cuda";
    if (disp->cfg.gpu_backend == GPU_BACKEND_OPENCL) return "opencl";
    return "none";
}
