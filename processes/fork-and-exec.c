#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
    pid_t child = fork();
    if (child < 0) {
        fprintf(stderr, "Failed to fork: %d\n", child);
        return -1;
    } else if (child == 0) {
        // child
        pid_t child_pid = getpid();
        printf("Child [%d] about to call exec()\n", child_pid);
        char *args[] = {"noopz", "arg1", "arg2", NULL};
        if (execvp("./noop", args) == -1) {
            fprintf(stderr, "exec() failed: %s\n", strerror(errno));
            return -1;
        };
        printf("We will never reach here\n");
    } else {
        // parent
        pid_t parent = getpid();
        printf("%d Forked child %d\n", parent, child);
    }
}
