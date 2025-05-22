#include "util.h"
#include <sys/mman.h>
#include <math.h>

#define CACHE_SETS 32768
#define L3_ASSOC 12
#define L3_SIZE 25165824
#define REDUNDANCY 5
#define BUF_SIZE L3_SIZE*REDUNDANCY
#define THERESHOLD 100
#define POTENTIAL_LINES BUF_SIZE / (2<<17)

#define REPEAT 5

void set_subtraction(void **A, size_t A_size, void **B, size_t B_size, void *C) {
    int C_size=0;
    for (int i=0; i<A_size; i++) {
        int found = 0;
        for (int j=0; j<B_size; j++) {
            if (((uint64_t*)A)[i] == ((uint64_t *)B)[j]) {
                found = 1;
                break;
            }
        }
        if (!found) 
            ((uint64_t*)C)[C_size++] = ((uint64_t*)A)[i];
    }
}

void shuffle(void **array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          void  *t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void get_potential_conflict_lines(void *buf, void **potential_conflict_lines, uint64_t set) {
    int i=0;
    while (buf + (uint64_t)(i<<18) + (uint64_t)(set<<6) < buf+BUF_SIZE) {
        potential_conflict_lines[i] = buf + (uint64_t)(i<<18) + (uint64_t)(set<<6);
        i++;
    }

}

int probe(void **set, size_t set_size, void *candidate) {
    char tmp;
    uint32_t time;

    tmp = *((char *) candidate);

    for (int k=0; k<REPEAT; k++) {
        for (int i=0; i<set_size; i++) {
            if (set[i] != candidate)
                tmp = *((char *) set[i]);
        }
    }

    time = measure_one_block_access_time((uint64_t) candidate);
    //printf("time: %d\n", time);

    return time > THERESHOLD;
}

int probe2(void **set, size_t set_size, void *candidate, void *exclude) {
    char tmp;
    uint32_t time;

    tmp = *((char *) candidate);

    for (int k=0; k<REPEAT; k++) {
        for (int i=0; i<set_size; i++) {
            if (set[i] != exclude)
                tmp = *((char *) set[i]);
        }
    }

    time = measure_one_block_access_time((uint64_t) candidate);
    //("time: %d\n", time);
    return time > THERESHOLD;
}

int main() {
    void *buf= mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, 
        MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, 
        -1, 0);
        
    void *potential_conflict_lines[POTENTIAL_LINES];
    
    get_potential_conflict_lines(buf, potential_conflict_lines, 0x0);
    shuffle(potential_conflict_lines, BUF_SIZE / (2<<17));
    
    size_t conflict_set_size = 0;
    void *conflict_set[POTENTIAL_LINES];
    for (int i=0; i<POTENTIAL_LINES; i++) {
        void *candidate = potential_conflict_lines[i];
        if (!probe(conflict_set, conflict_set_size, candidate)) {
            conflict_set[conflict_set_size++] = candidate;
            //printf("%p\n", conflict_set[conflict_set_size-1]);
        }
    }

    printf("lines: %d\n", BUF_SIZE / (2<<17));
    printf("conflict_set_size: %d\n", conflict_set_size);

    //void *potential_conflict_lines2[POTENTIAL_LINES];
    //
    //get_potential_conflict_lines(buf, potential_conflict_lines2, 0x0);
    //shuffle(potential_conflict_lines2, BUF_SIZE / (2<<17));
    //
    //size_t conflict_set_size2 = 0;
    //void *conflict_set2[POTENTIAL_LINES];
    //for (int i=0; i<POTENTIAL_LINES; i++) {
    //    void *candidate = potential_conflict_lines2[i];
    //    if (!probe(conflict_set2, conflict_set_size2, candidate)) {
    //        conflict_set2[conflict_set_size2++] = candidate;
    //        //printf("%p\n", conflict_set[conflict_set_size-1]);
    //    }
    //}
    //
    //
    //printf("lines: %d\n", BUF_SIZE / (2<<17));
    //printf("conflict_set_size: %d\n", conflict_set_size2);
    //
    //for (int i=0; i<192; i++) {
    //    printf("%p\t%p\n", conflict_set[i], conflict_set2[i]);
    //}
#if 1

    return 0;
}
#else 

    void *not_conflict_set[POTENTIAL_LINES - conflict_set_size];
    set_subtraction(potential_conflict_lines, POTENTIAL_LINES, conflict_set, conflict_set_size, not_conflict_set);

    for (int i = 0; i < POTENTIAL_LINES - conflict_set_size; i++) {
        void *candidate = not_conflict_set[i];
        if (probe(conflict_set, conflict_set_size, candidate)) {
            int eviction_set_size = 0;
            for (int j=0; j<conflict_set_size; j++) {
                if (!probe2(conflict_set, conflict_set_size, candidate, conflict_set[j])) {
                    eviction_set_size++;
                }
            }
            printf("eviction_set_size: %d\n", eviction_set_size);
        }
        break;
        
    }
    

}
#endif

