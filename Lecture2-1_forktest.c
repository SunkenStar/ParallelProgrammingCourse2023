#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int glob = 6;
int main()
{
    pid_t pid;
    int x = 1;
    pid = fork();
    if (pid < 0) {
        printf("%s\n", strerror(errno));
    } else if (pid == 0) {
        glob++;
        printf("child process: glob=%d\n, x = %d\n", glob, ++x);
        exit(0);
    }
    printf("parent process: glob=%d\n, x = %d\n", glob, --x);
    return EXIT_SUCCESS;
}