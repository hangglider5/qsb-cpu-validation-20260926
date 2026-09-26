// CPU-only driver around the complete upstream host header and exact gate.
#include "generated/host_support.h"
#ifdef BASELINE
#include "generated/baseline_cpu.h"
#else
#include "generated/candidates/subset/CpuGrindSubset.h"
#endif
int main(int argc,char **argv){
    if(argc!=2)return 2;digest_params_t dp;
    if(load_digest_params(argv[1],&dp))return 3;
    __builtin_cpu_init();
    if(!__builtin_cpu_supports("avx512f") || !__builtin_cpu_supports("avx512ifma")){
        printf("REQUIRED IFMA UNAVAILABLE\n");return 4;
    }
    uint8_t win[128][3];int n=0;
    for(int a=0;a<13;++a)for(int b=a+1;b<13;++b)for(int c=b+1;c<13;++c)
        if(a>=6 || (a<=5 && b>=7) || (a==0 && b==1 && c>=8 && c<=10)){
            win[n][0]=137+a;win[n][1]=137+b;win[n][2]=137+c;++n;
        }
    if(n!=128)return 5;
    qcpu::start(&dp,win,128,137,6);
    return 6; // dev hook exits only after completion; unexpected return is failure.
}
