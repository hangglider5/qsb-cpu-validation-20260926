// Independent baseline arithmetic checks against OpenSSL; no GPU or score claim.
#include "baseline_primitives.h"
#include <random>
#include <chrono>
#include <iostream>
using namespace qcpu;
static std::mt19937_64 rng(20260926);
static fe random_fe() {
    fe a = {{rng(), rng(), rng(), rng()}};
    if (fe_ge_p(a.v)) fe_sub_p(a.v);
    return a;
}
static void check(bool ok, const char *what) {
    if (!ok) { std::cerr << "FAIL " << what << '\n'; exit(1); }
}
static void scalar_checks() {
    BN_CTX *ctx = BN_CTX_new(); BIGNUM *p = nullptr;
    BN_hex2bn(&p, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
    BIGNUM *a = BN_new(), *b = BN_new(), *expected = BN_new();
    for (int i = 0; i < 20000; ++i) {
        fe x = random_fe(), y = random_fe(), z, ref;
        if (i < 8) { x = {{(uint64_t)i,0,0,0}}; y = {{P0-(uint64_t)i-1,~0ULL,~0ULL,~0ULL}}; }
        BN_lebin2bn((const unsigned char *)x.v, 32, a);
        BN_lebin2bn((const unsigned char *)y.v, 32, b);
        BN_mod_mul(expected, a, b, p, ctx); fe_from_bn(ref, expected);
        fe_mul(z,x,y); check(fe_eq(z,ref), "scalar mul vs BN");
        fe_mul(x,x,y); check(fe_eq(x,ref), "scalar mul alias");
        x = random_fe(); BN_lebin2bn((const unsigned char *)x.v,32,a);
        BN_mod_add(expected,a,b,p,ctx); fe_from_bn(ref,expected);
        fe_add(z,x,y); check(fe_eq(z,ref),"scalar add vs BN");
        BN_mod_sub(expected,a,b,p,ctx); fe_from_bn(ref,expected);
        fe_sub(z,x,y); check(fe_eq(z,ref),"scalar sub vs BN");
        if (i < 256) {
            check(BN_mod_inverse(expected,a,p,ctx) != nullptr,"BN inverse");
            fe_from_bn(ref,expected); fe_inv(z,x); check(fe_eq(z,ref),"scalar inv vs BN");
        }
    }
    BN_free(a); BN_free(b); BN_free(expected); BN_free(p); BN_CTX_free(ctx);
    std::cout << "PASS scalar OpenSSL reference: 20000 operand pairs, 256 inverses\n";
}
#if QCPU_VEC
Q8T static void pack(fe8 &v, const fe *a) {
    alignas(64) uint64_t w[4][8];
    for (int k=0;k<4;++k) for (int j=0;j<8;++j) w[k][j]=a[j].v[k];
    fe8_from64(v,_mm512_load_si512(w[0]),_mm512_load_si512(w[1]),_mm512_load_si512(w[2]),_mm512_load_si512(w[3]));
}
Q8T static void vector_checks() {
    fe a[8],b[8],out[8],ref;
    for (int i=0;i<20000;++i) {
        for (int j=0;j<8;++j) { a[j]=random_fe();b[j]=random_fe(); }
        if (i < 4) for (int j=0;j<8;++j) {
            a[j]={{(uint64_t)j,0,0,0}}; b[j]={{P0-(uint64_t)j-1,~0ULL,~0ULL,~0ULL}};
        }
        fe8 x,y,z; pack(x,a);pack(y,b);
        fe8_mul(z,x,y);fe8_store_canon(out,z);
        for(int j=0;j<8;++j){fe_mul(ref,a[j],b[j]);check(fe_eq(out[j],ref),"IFMA mul");}
        fe8_sqr(z,x);fe8_store_canon(out,z);
        for(int j=0;j<8;++j){fe_sqr(ref,a[j]);check(fe_eq(out[j],ref),"IFMA sqr");}
        fe8_add(z,x,y);fe8_store_canon(out,z);
        for(int j=0;j<8;++j){fe_add(ref,a[j],b[j]);check(fe_eq(out[j],ref),"IFMA add");}
        fe8_sub(z,x,y);fe8_store_canon(out,z);
        for(int j=0;j<8;++j){fe_sub(ref,a[j],b[j]);check(fe_eq(out[j],ref),"IFMA sub");}
        __m512i cw[4]; fe8_canon_words(cw,z);alignas(64) uint64_t w[4][8];
        for(int k=0;k<4;++k)_mm512_store_si512(w[k],cw[k]);
        for(int j=0;j<8;++j)for(int k=0;k<4;++k)check(w[k][j]==out[j].v[k],"vector canonicalization");
    }
    for(int i=0;i<128;++i){
        fe8 x[4];fe inputs[4][8];
        for(int k=0;k<4;++k){for(int j=0;j<8;++j)inputs[k][j]=random_fe();pack(x[k],inputs[k]);}
        fe8_inv4(x);
        for(int k=0;k<4;++k){fe8_store_canon(out,x[k]);for(int j=0;j<8;++j){fe_mul(ref,out[j],inputs[k][j]);check(ref.v[0]==1 && !ref.v[1] && !ref.v[2] && !ref.v[3],"IFMA inv4");}}
    }
    std::cout << "PASS IFMA: 160000 lanes per operation; 4096 inverses\n";
}
#endif
#if QCPU_SHANI
static void hash_checks() {
    for(int i=0;i<10000;++i){
        alignas(16) uint32_t st[4][8],orig[4][8],wk[4][64],words[16];
        alignas(16) uint8_t block[4][64];const uint8_t *bp[4];const uint32_t *wp[4];
        for(int l=0;l<4;++l){
            for(int j=0;j<8;++j)st[l][j]=orig[l][j]=(uint32_t)rng();
            for(int j=0;j<64;++j)block[l][j]=(uint8_t)rng();
            for(int j=0;j<16;++j)words[j]=((uint32_t)block[l][4*j]<<24)|((uint32_t)block[l][4*j+1]<<16)|((uint32_t)block[l][4*j+2]<<8)|block[l][4*j+3];
            qsha_sched(wk[l],words);bp[l]=block[l];wp[l]=wk[l];
        }
        qsha_x4_run(st,wp,1);
        for(int l=0;l<4;++l){SHA256_CTX ctx;SHA256_Init(&ctx);for(int j=0;j<8;++j)ctx.h[j]=orig[l][j];SHA256_Transform(&ctx,block[l]);for(int j=0;j<8;++j)check(st[l][j]==ctx.h[j],"SHA precomputed vs OpenSSL");}
        memcpy(st,orig,sizeof(st));qsha_x4(st,bp);
        for(int l=0;l<4;++l){SHA256_CTX ctx;SHA256_Init(&ctx);for(int j=0;j<8;++j)ctx.h[j]=orig[l][j];SHA256_Transform(&ctx,block[l]);for(int j=0;j<8;++j)check(st[l][j]==ctx.h[j],"SHA x4 vs OpenSSL");}
    }
    std::cout << "PASS SHA-NI: 40000 random lane-blocks for both routines\n";
}
#endif
int main(){
    auto start=std::chrono::steady_clock::now();scalar_checks();
#if QCPU_VEC
    __builtin_cpu_init();
    if(__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512ifma"))vector_checks();
    else std::cout << "SKIP IFMA runtime: unavailable; target functions compiled\n";
#endif
#if QCPU_SHANI
    if(qsha_supported())hash_checks();else std::cout<<"SKIP SHA-NI runtime: unavailable\n";
#endif
    std::cout<<"ALL EXECUTED CHECKS PASS; elapsed_s="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n";
}
