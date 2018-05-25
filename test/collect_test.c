//
// Created by Srdan Milakovic on 5/15/18.
//

#include "collect.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util/util.h"
#include "util/debug.h"
#include "util/run.h"

#define VERIFY
#define PRINTx


typedef void (*collect_impl)(void *, const void *, size_t, int, int, int, long *);

static inline void shcoll_collect32_shmem(void *dest, const void *source, size_t nelems, int PE_start,
                                          int logPE_stride, int PE_size, long *pSync) {
    shmem_collect32(dest, source, nelems, PE_start, logPE_stride, PE_size, pSync);
}

double test_collect32(collect_impl collect, int iterations, size_t count, long SYNC_VALUE, size_t COLLECT_SYNC_SIZE) {
    long *pSync = shmem_malloc(COLLECT_SYNC_SIZE * sizeof(long));
    for (int i = 0; i < COLLECT_SYNC_SIZE; i++) {
        pSync[i] = SYNC_VALUE;
    }

    shmem_barrier_all();

    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    size_t nelems = count * (me + 1);

    size_t total_size = (size_t) (count * (1 + npes) * npes / 2);

    uint32_t *dst = shmem_malloc(total_size * sizeof(uint32_t));
    uint32_t *src = shmem_malloc(count * npes * sizeof(uint32_t));

    for (int i = 0; i < nelems; i++) {
        src[i] = (uint32_t) ((me + 1) * (i + 1));
    }

    shmem_barrier_all();
    unsigned long long start = current_time_ns();

    for (int i = 0; i < iterations; i++) {
        #ifdef VERIFY
        memcpy(dst, src, total_size * sizeof(uint32_t));
        #endif

        shmem_sync_all();
        collect(dst, src, nelems, 0, 0, npes, pSync);

        #ifdef PRINT
        for (int b = 0; b < npes; b++) {
            shmem_barrier_all();
            if (b != me) {
                continue;
            }

            gprintf("%d: %2d", shmem_my_pe(), dst[0]);
            for (int j = 1; j < total_size; j++) { gprintf(", %2d", dst[j]); }
            gprintf("\n");
        }
        #endif

        #ifdef VERIFY
        int j = 0;
        for (int p = 0; p < npes; p++) {
            for (int k = 0; k < (p + 1) * count; k++, j++) {
                if (dst[j] != (p + 1) * (k + 1)) {
                    gprintf("[%d] dst[%d] = %d; Expected %d\n", shmem_my_pe(), j, dst[j], (p + 1) * (k + 1));
                    abort();
                }
            }
        }
        #endif
    }

    shmem_barrier_all();
    unsigned long long end = current_time_ns();

    shmem_free(pSync);
    shmem_free(src);
    shmem_free(dst);

    return (end - start) / 1e9;

}


int main(int argc, char *argv[]) {
    int iterations = (int) (argc > 1 ? strtol(argv[1], NULL, 0) : 1);
    size_t count = (size_t) (argc > 2 ? strtol(argv[2], NULL, 0) : 1);

    shmem_init();
    int npes = shmem_n_pes();

    if (shmem_my_pe() == 0) {
        gprintf("[%s]PEs: %d\n", __FILE__, shmem_n_pes());
    }

    RUN(collect32, shmem, iterations, count, SHMEM_SYNC_VALUE, SHMEM_COLLECT_SYNC_SIZE);
    RUN(collect32, linear, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);
    RUN(collect32, ring, iterations, count, SHCOLL_SYNC_VALUE, SHCOLL_COLLECT_SYNC_SIZE);

    shmem_finalize();
}