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
        fprintf(stderr, "Allocation failed!\n");
        return -2;
    }

    sleep(200);
}
