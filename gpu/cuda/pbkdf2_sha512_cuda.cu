/*
 * CUDA BIP39 PBKDF2-HMAC-SHA512 batch.
 * Device kernels run one mnemonic per thread; SHA512-HMAC is device-side.
 * Falls back to host if no CUDA device.
 */
#include "pbkdf2_sha512_cuda.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Compact device SHA-512 (FIPS 180-4) — enough for BIP39 PBKDF2. */
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

__device__ static u64 rotr64(u64 x, int n) { return (x >> n) | (x << (64 - n)); }

__device__ static const u64 K512[80] = {
	0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
	0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
	0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
	0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
	0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
	0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
	0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
	0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
	0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
	0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
	0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
	0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
	0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
	0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
	0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
	0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
	0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
	0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
	0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
	0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL
};

struct Sha512Ctx {
	u64 h[8];
	u64 bitlen;
	u8 data[128];
	u32 datalen;
};

__device__ static void sha512_init(Sha512Ctx *ctx) {
	ctx->datalen = 0; ctx->bitlen = 0;
	ctx->h[0]=0x6a09e667f3bcc908ULL; ctx->h[1]=0xbb67ae8584caa73bULL;
	ctx->h[2]=0x3c6ef372fe94f82bULL; ctx->h[3]=0xa54ff53a5f1d36f1ULL;
	ctx->h[4]=0x510e527fade682d1ULL; ctx->h[5]=0x9b05688c2b3e6c1fULL;
	ctx->h[6]=0x1f83d9abfb41bd6bULL; ctx->h[7]=0x5be0cd19137e2179ULL;
}

__device__ static void sha512_transform(Sha512Ctx *ctx, const u8 data[]) {
	u64 m[80], a,b,c,d,e,f,g,h,t1,t2;
	for(int i=0,j=0;i<16;++i,j+=8)
		m[i]=((u64)data[j]<<56)|((u64)data[j+1]<<48)|((u64)data[j+2]<<40)|((u64)data[j+3]<<32)|
		     ((u64)data[j+4]<<24)|((u64)data[j+5]<<16)|((u64)data[j+6]<<8)|((u64)data[j+7]);
	for(int i=16;i<80;++i) {
		u64 s0=rotr64(m[i-15],1)^rotr64(m[i-15],8)^(m[i-15]>>7);
		u64 s1=rotr64(m[i-2],19)^rotr64(m[i-2],61)^(m[i-2]>>6);
		m[i]=m[i-16]+s0+m[i-7]+s1;
	}
	a=ctx->h[0];b=ctx->h[1];c=ctx->h[2];d=ctx->h[3];
	e=ctx->h[4];f=ctx->h[5];g=ctx->h[6];h=ctx->h[7];
	for(int i=0;i<80;++i){
		u64 S1=rotr64(e,14)^rotr64(e,18)^rotr64(e,41);
		u64 ch=(e&f)^((~e)&g);
		t1=h+S1+ch+K512[i]+m[i];
		u64 S0=rotr64(a,28)^rotr64(a,34)^rotr64(a,39);
		u64 maj=(a&b)^(a&c)^(b&c);
		t2=S0+maj;
		h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
	}
	ctx->h[0]+=a;ctx->h[1]+=b;ctx->h[2]+=c;ctx->h[3]+=d;
	ctx->h[4]+=e;ctx->h[5]+=f;ctx->h[6]+=g;ctx->h[7]+=h;
}

__device__ static void sha512_update(Sha512Ctx *ctx, const u8 *data, size_t len) {
	for(size_t i=0;i<len;++i){
		ctx->data[ctx->datalen++]=data[i];
		if(ctx->datalen==128){
			sha512_transform(ctx,ctx->data);
			ctx->bitlen+=1024;
			ctx->datalen=0;
		}
	}
}

__device__ static void sha512_final(Sha512Ctx *ctx, u8 hash[64]) {
	u32 i=ctx->datalen;
	ctx->data[i++]=0x80;
	if(i>112){
		while(i<128) ctx->data[i++]=0;
		sha512_transform(ctx,ctx->data);
		i=0;
	}
	while(i<112) ctx->data[i++]=0;
	ctx->bitlen+=ctx->datalen*8ULL;
	/* 128-bit length; high 64 zero */
	for(int j=0;j<8;j++) ctx->data[127-j]=(u8)((ctx->bitlen>>(8*j))&0xff);
	for(int j=0;j<8;j++) ctx->data[119-j]=0;
	sha512_transform(ctx,ctx->data);
	for(int j=0;j<8;j++){
		hash[j*8+0]=(u8)(ctx->h[j]>>56); hash[j*8+1]=(u8)(ctx->h[j]>>48);
		hash[j*8+2]=(u8)(ctx->h[j]>>40); hash[j*8+3]=(u8)(ctx->h[j]>>32);
		hash[j*8+4]=(u8)(ctx->h[j]>>24); hash[j*8+5]=(u8)(ctx->h[j]>>16);
		hash[j*8+6]=(u8)(ctx->h[j]>>8);  hash[j*8+7]=(u8)(ctx->h[j]);
	}
}

__device__ static void d_memcpy(u8 *d, const u8 *s, int n) { for(int i=0;i<n;i++) d[i]=s[i]; }
__device__ static void d_memset(u8 *d, u8 v, int n) { for(int i=0;i<n;i++) d[i]=v; }

__device__ static void hmac_sha512(const u8 *key, int keylen, const u8 *msg, int msglen, u8 out[64]) {
	u8 k[128];
	d_memset(k,0,128);
	if(keylen>128){
		Sha512Ctx c; sha512_init(&c); sha512_update(&c,key,(size_t)keylen); sha512_final(&c,k);
	} else {
		d_memcpy(k,key,keylen);
	}
	u8 ipad[128], opad[128];
	for(int i=0;i<128;i++){ ipad[i]=k[i]^0x36; opad[i]=k[i]^0x5c; }
	Sha512Ctx ctx;
	u8 inner[64];
	sha512_init(&ctx); sha512_update(&ctx,ipad,128); sha512_update(&ctx,msg,(size_t)msglen); sha512_final(&ctx,inner);
	sha512_init(&ctx); sha512_update(&ctx,opad,128); sha512_update(&ctx,inner,64); sha512_final(&ctx,out);
}

__device__ static void pbkdf2_hmac_sha512_d(const u8 *pw, int pwlen, const u8 *salt, int saltlen,
                                           int iters, u8 *out, int outlen) {
	u8 U[64], T[64], block[128];
	int blocks = (outlen + 63) / 64;
	for(int i=1;i<=blocks;i++){
		d_memcpy(block, salt, saltlen);
		block[saltlen+0]=(u8)(i>>24); block[saltlen+1]=(u8)(i>>16);
		block[saltlen+2]=(u8)(i>>8);  block[saltlen+3]=(u8)i;
		hmac_sha512(pw, pwlen, block, saltlen+4, U);
		d_memcpy(T,U,64);
		for(int j=1;j<iters;j++){
			hmac_sha512(pw, pwlen, U, 64, U);
			for(int k=0;k<64;k++) T[k]^=U[k];
		}
		int off=(i-1)*64;
		int n = outlen-off; if(n>64) n=64;
		d_memcpy(out+off, T, n);
	}
}

__global__ static void k_pbkdf2(const char *blob, const uint32_t *off, const uint32_t *lens,
                                uint32_t count, const char *pass, int passlen, uint8_t *seeds) {
	uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
	if(idx >= count) return;
	const char *mn = blob + off[idx];
	uint32_t mlen = lens[idx];
	char salt[96];
	int sl = 0;
	const char *pref = "mnemonic";
	for(int i=0; pref[i] && sl<80; i++) salt[sl++]=pref[i];
	for(int i=0; i<passlen && sl<95; i++) salt[sl++]=pass[i];
	pbkdf2_hmac_sha512_d((const u8*)mn, (int)mlen, (const u8*)salt, sl, 2048, seeds + (size_t)idx*64, 64);
}

extern "C" int tcuda_pbkdf2_sha512_batch(const char *blob, const uint32_t *offsets,
                                         const uint32_t *lens, uint32_t count,
                                         const char *passphrase, uint8_t *out_seeds64) {
	if(!blob || !offsets || !lens || !out_seeds64 || count == 0) return 0;
	int ndev = 0;
	if(cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
		fprintf(stderr, "[W] CUDA PBKDF2: no device; use host PBKDF2\n");
		return 0;
	}
	uint32_t blob_bytes = 0;
	for(uint32_t i = 0; i < count; i++) {
		uint32_t end = offsets[i] + lens[i];
		if(end > blob_bytes) blob_bytes = end;
	}
	char *d_blob = nullptr; uint32_t *d_off = nullptr, *d_len = nullptr;
	char *d_pass = nullptr; uint8_t *d_seeds = nullptr;
	int plen = passphrase ? (int)strlen(passphrase) : 0;
	if(cudaMalloc(&d_blob, blob_bytes) != cudaSuccess) return 0;
	if(cudaMalloc(&d_off, count * sizeof(uint32_t)) != cudaSuccess) { cudaFree(d_blob); return 0; }
	if(cudaMalloc(&d_len, count * sizeof(uint32_t)) != cudaSuccess) { cudaFree(d_blob); cudaFree(d_off); return 0; }
	if(cudaMalloc(&d_pass, (size_t)plen + 1) != cudaSuccess) { cudaFree(d_blob); cudaFree(d_off); cudaFree(d_len); return 0; }
	if(cudaMalloc(&d_seeds, (size_t)count * 64) != cudaSuccess) {
		cudaFree(d_blob); cudaFree(d_off); cudaFree(d_len); cudaFree(d_pass); return 0;
	}
	cudaMemcpy(d_blob, blob, blob_bytes, cudaMemcpyHostToDevice);
	cudaMemcpy(d_off, offsets, count * sizeof(uint32_t), cudaMemcpyHostToDevice);
	cudaMemcpy(d_len, lens, count * sizeof(uint32_t), cudaMemcpyHostToDevice);
	cudaMemcpy(d_pass, passphrase ? passphrase : "", (size_t)plen + 1, cudaMemcpyHostToDevice);
	int threads = 64;
	int blocks = (int)((count + threads - 1) / threads);
	k_pbkdf2<<<blocks, threads>>>(d_blob, d_off, d_len, count, d_pass, plen, d_seeds);
	cudaError_t err = cudaDeviceSynchronize();
	int ok = (err == cudaSuccess);
	if(ok) cudaMemcpy(out_seeds64, d_seeds, (size_t)count * 64, cudaMemcpyDeviceToHost);
	else fprintf(stderr, "[E] CUDA PBKDF2 kernel: %s\n", cudaGetErrorString(err));
	cudaFree(d_blob); cudaFree(d_off); cudaFree(d_len); cudaFree(d_pass); cudaFree(d_seeds);
	if(ok) printf("[+] CUDA PBKDF2-HMAC-SHA512 batch: %u mnemonics\n", count);
	return ok;
}
