#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();
    /*child sleeps until SIGKILL signal received, then dies*/
    if (pid < 0) {
        printf("%s\n", strerror(errno));
        return EXIT_FAILURE;
    } else if (pid == 0) {
        printf("Child is waiting for signal.\n");
        pause();
        printf("this sentence should never be printed\n");
        /*Even if the SIGKILL is replaced with SIGTERM*/
        exit(0);
    }
    /*parent sends a SIGKILL signal to a child*/
    for (int i = 0; i < 0xfffffff; i++) {
        if (i % 0xffffff == 0)
            printf("waiting...\n");
    }
    /*Without this waiting step the child will be killed before printing the first sentence.*/
    kill(pid, SIGKILL);
    printf("parent has sent a SIGKILL signal.\n");
    return EXIT_SUCCESS;
}