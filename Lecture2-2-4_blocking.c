#define _POSIX_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int sign = 0;

void receive(int signo)
{
    printf("pid=%d,received\n", getpid());
    sign = 1;
}

int main()
{
    pid_t pid = getpid();
    sigset_t chldmask, savemask;
    sigemptyset(&chldmask);
    sigaddset(&chldmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &chldmask, &savemask);
    if ((pid = fork()) == 0) {
        printf("%d\n", getpid());
        signal(SIGUSR1, receive); // 注册SIGUSR1的响应器
        sigprocmask(SIG_SETMASK, &savemask, NULL);
        while (sign == 0)
            ;                     // 原地踏步程序;
        sign = 0;                 // 还原sign值
        kill(getppid(), SIGUSR1); // 给父进程发信号
        exit(0);
    }
    signal(SIGUSR1, receive); // 注册SIGUSR1的响应器
    sigprocmask(SIG_SETMASK, &savemask, NULL);
    // here, do something
    kill(pid, SIGUSR1);
    printf("SIGUSR1 has been sent\n");
    while (sign == 0)
        ;     // 原地踏步程序,等待子进程的接受确认消息
    sign = 0; // 还原sign值
    return EXIT_SUCCESS;
}

// The sigprocmasks are used to make the process hold until recieve function gets online.