#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv) {
    pid_t child = vfork();

    if (child < 0) {
        fprintf(stderr, "Failed to fork, vfork() returned %d\n", child);
        return -1;
    } else if(child == 0) {
        printf("vforked, child sleeping\n");
        sleep(30);
    } else {
        printf("vforked. child pid: %d\n", child);
    }

}
