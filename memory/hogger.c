#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Please specify memory in MB\n");
        return -1;
    }

    pid_t pid = getpid();
    int PAGE_SIZE = getpagesize();
    printf("[%d] Page size: %d\n", pid, PAGE_SIZE);
    int mb = atoi(argv[1]);
    long SIZE = (long)mb*(1L<<20);
    printf("[%d] Allocating %dMB (%ld bytes)\n", pid, mb, SIZE);
    char *buff = malloc(SIZE);
    if (buff == NULL) {
        fprinf(stderr, "Allocation failed!\n");
        return -2;
    }
    printf("[%d] Dirtying pages\n", pid);
    for (long offset = 0; offset < SIZE; offset += PAGE_SIZE) {
        buff[offset] = 0;

        if (offset % (PAGE_SIZE*1024) == 0) {
            printf("[%d] Dirtied %ldMB\n", pid, offset / (1L<<20));
        }
    }
    
    if (argc == 3 && strcmp("-f", argv[2]) == 0) {
        pid_t child = fork();
        if (child < 0) {
            fprintf(stderr, "[%d] Fork failed, %s\n", pid, strerror(errno));
        } else if (child == 0) // child
        {
            pid = getpid();
            printf("[%d] Slowly dirtying memory\n", pid);
            // slowly dirty memory
            for (long offset = 0; offset < SIZE; offset += PAGE_SIZE) {
                buff[offset] = 1;
                if (offset % (PAGE_SIZE*1024) == 0) {
                    printf("[%d] Dirtied %ldMB\n", pid, offset / (1L<<20));
                    sleep(1);
                }
            }
        } else {
            printf("[%d] Parent; Sleeping; Child pid: %d\n", pid, child);
        }
        
    }
    sleep(200);
}
