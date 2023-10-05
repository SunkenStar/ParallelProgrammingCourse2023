#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

void handler(int sig)
{
    if (sig == SIGKILL) {
        printf("That's it, I'm dead.\n");
    } /* But SIGKILL doesn't get captured, it just kills the process
         before the handler reacts. So this sentence can never be output.*/
    else if (sig == SIGTERM) {
        printf("Catch signal SIGTERM\n");
        exit(0);
    } else {
        printf("Sorry! you are signal %d, i will not exit!\n", sig);
    }
}

int main()
{
    pid_t pid;
    if ((pid = fork()) == 0) {
        signal(SIGKILL, handler);
        signal(SIGTERM, handler);
        signal(SIGINT, handler);
        printf("Ready to recieve signal\n");
        while (1)
            pause();
    }
    /*父进程向子进程发送信号 */
    sleep(2);
    kill(pid, SIGINT);
    sleep(2);
    // kill(pid, SIGTERM);
    kill(pid, SIGKILL);
    return EXIT_SUCCESS;
}