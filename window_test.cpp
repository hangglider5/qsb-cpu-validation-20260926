// Scheduling comparison over synthetic field operands. Full hit sets tested separately.
#include "generated/primitive_round3.h"
#include <random>
#include <chrono>
#include <iostream>
using namespace qcpu;
using clock_type=std::chrono::steady_clock;
static std::mt19937_64 rng(2026092603);
static volatile uint64_t sink;
static fe rand_fe(){fe a={{rng(),rng(),rng(),rng()}};if(fe_ge_p(a.v))fe_sub_p(a.v);return a;}
Q8T static void pack(fe8 &v){
    alignas(64)uint64_t w[4][8];for(int j=0;j<8;++j){fe a=rand_fe();for(int i=0;i<4;++i)w[i][j]=a.v[i];}
    fe8_from64(v,_mm512_load_si512(w[0]),_mm512_load_si512(w[1]),_mm512_load_si512(w[2]),_mm512_load_si512(w[3]));
}
using buf=std::vector<fe8,qalloc64<fe8>>;
Q8T static void checks(){
    for(int G:{4,12,16,128,512})for(int trial=0;trial<8;++trial){
        buf X(G),Y(G),A(G),B(G),D(2*G),P(2*G),T(2*G);
        std::vector<pt,qalloc64<pt>> rows(G*8);
        for(int h=0;h<G;++h){pack(X[h]);pack(Y[h]);}
        for(auto &r:rows){r.x=rand_fe();r.y=rand_fe();}
        auto rowfn=[&](int h,const pt **out)->__mmask8{for(int j=0;j<8;++j)out[j]=&rows[h*8+j];return (h*73+trial)&255;};
        if(trial==7){fe ax[8];fe8_store_canon(ax,X[0]);rows[0].x=ax[0];}
        memcpy(A.data(),X.data(),G*sizeof(fe8));memcpy(B.data(),Y.data(),G*sizeof(fe8));
        ec8_window_chains(A.data(),B.data(),D.data(),P.data(),T.data(),G,rowfn);
        ec8_window(X.data(),Y.data(),D.data(),P.data(),T.data(),G,rowfn);
        for(int h=0;h<G;++h){fe x[8],y[8],a[8],b[8];fe8_store_canon(x,X[h]);fe8_store_canon(y,Y[h]);fe8_store_canon(a,A[h]);fe8_store_canon(b,B[h]);
            for(int j=0;j<8;++j)if(!fe_eq(x[j],a[j]) || !fe_eq(y[j],b[j])){std::cerr<<"FAIL window G="<<G<<" trial="<<trial<<"\n";exit(1);}}
    }
    std::cout<<"PASS canonical window outputs: G=4/12/16/128/512, 8 inputs each, signed rows and zero denominator\n";
}
Q8T static void bench(){
    const int G=512;const size_t N=(64ULL<<20)/sizeof(pt);
    std::vector<pt,qalloc64<pt>> table(N);for(auto &r:table){r.x=rand_fe();r.y=rand_fe();}
    // Rotate addresses so repeated batches do not collapse into 256 KiB of hot rows.
    std::vector<uint32_t> idx(512*G*8);for(auto &i:idx)i=rng()%N;size_t active=0;
    buf X(G),Y(G),A(G),B(G),D(2*G),P(2*G),T(2*G);for(int h=0;h<G;++h){pack(A[h]);pack(B[h]);}
    auto rowfn=[&](int h,const pt **out)->__mmask8{for(int j=0;j<8;++j)out[j]=&table[idx[active+h*8+j]];return (h*73)&255;};
    auto trial=[&](bool tree,double seconds){
        size_t n=0;auto start=clock_type::now();double el=0;
        do{active=(n%512)*G*8;memcpy(X.data(),A.data(),G*sizeof(fe8));memcpy(Y.data(),B.data(),G*sizeof(fe8));
            if(tree)ec8_window(X.data(),Y.data(),D.data(),P.data(),T.data(),G,rowfn);
            else ec8_window_chains(X.data(),Y.data(),D.data(),P.data(),T.data(),G,rowfn);
            uint64_t w;memcpy(&w,&X[0].l[0],sizeof(w));sink=w;++n;
            el=std::chrono::duration<double>(clock_type::now()-start).count();
        }while(el<seconds);
        return el*1e9/(n*G*8);
    };
    trial(false,.2);trial(true,.2);
    for(int block=0;block<4;++block)for(bool tree:{false,true,true,false})
        std::cout<<"MICRO block="<<block<<" arm="<<(tree?"tree":"chains")<<" ns_per_candidate="<<trial(tree,.5)<<" G="<<G<<" address_pool_MiB=64\n";
}
int main(){__builtin_cpu_init();if(!__builtin_cpu_supports("avx512f") || !__builtin_cpu_supports("avx512ifma")){std::cerr<<"required IFMA unavailable\n";return 4;}checks();bench();}
