#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig)
{
    static int beeps = 0;
    printf("BEEP\n");
    if (++beeps < 5)
        alarm(1);
    /* 1秒后再次发送信号*/
    else {
        printf("BOOM!\n");
        exit(0);
    }
}

int main()
{
    signal(SIGALRM, handler);
    alarm(5); /* next SIGALRM will be delivered in 5s */
    printf("main func can do whatever you want;\n");
    while(1);/*But a infinite loop must be used to wait for the handler*/
    exit(0);
}