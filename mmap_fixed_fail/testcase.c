#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define PGSIZE 4096
#define len (PGSIZE * 3)

int main(int argc, char const *argv[]) {
    printf("mmap len: %d ...\n", len / PGSIZE);
    void *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        printf("mmap failed, err: %s\n", strerror(errno));
        return 1;
    }
    printf("mmap success, addr: %lx, touching mem...\n", (unsigned long)addr);
    for (int i = 0; i < len / PGSIZE; i++) {
        ((char *)addr)[1 + i * PGSIZE] = '1';
    }

    unsigned char vec[len / PGSIZE] = {0};

    printf("mincore ...\n");
    int ret = mincore(addr, len, vec);
    if (ret < 0) {
        printf("mincore failed, err: %s\n", strerror(errno));
        return -1;
    }
    for (int i = 0; i < len / PGSIZE; i++) {
        printf("addr + %4d: %s\n", i * PGSIZE, (vec[i] ? "incore" : "notincore"));
    }

    printf("sleeping...\n");
    sleep(1);

    const int len_fix = len - PGSIZE;
    printf("mmap fixed addr: %lx, len: %d ...\n", (unsigned long)addr, len_fix / PGSIZE);
    void *addr_fixed = mmap(addr, len_fix, PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    if (addr_fixed == MAP_FAILED) {
        printf("mmap fixed failed, error: %s\n", strerror(errno));

        printf("mincore ...\n");
        ret = mincore(addr, len, vec);
        if (ret < 0) {
            printf("mincore failed, err: %s\n", strerror(errno));
        } else {
            for (int i = 0; i < len / PGSIZE; i++) {
                printf("addr + %4d: %s\n", i * PGSIZE, (vec[i] ? "incore" : "notincore"));
            }
        }
        printf("Now touch mem, expect segmentation fault\n");
        for (int i = 0; i < len / PGSIZE; i++) {
            ((char *)addr)[1 + i * PGSIZE] = '1';
        }
    } else {
        printf("mmap fixed success, touching mem ...\n");
        for (int i = 0; i < len / PGSIZE; i++) {
            ((char *)addr)[1 + i * PGSIZE] = '1';
        }

        printf("mincore ...\n");
        ret = mincore(addr, len, vec);
        if (ret < 0) {
            printf("mincore failed, err: %s\n", strerror(errno));
            return -1;
        }
        for (int i = 0; i < len / PGSIZE; i++) {
            printf("addr + %4d: %s\n", i * PGSIZE, (vec[i] ? "incore" : "notincore"));
        }

        munmap(addr_fixed, len);
    }
    printf("Done.\n");
    return 0;
}
