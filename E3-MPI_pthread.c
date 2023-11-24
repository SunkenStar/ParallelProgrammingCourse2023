#include "mpi.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>

struct threadArg {
    int tid;        // 线程id
    double *B;      // B矩阵
    double *A_row;  // 得到的A中一行
    double *C_row;  // 得到的C中一行
    int numthreads; // 总线程数
    int N;          // 矩阵B的长
    int P;          // 矩阵B的宽
};

// 处理命令行参数，将两个待相乘的矩阵从文件读入程序，存入*fstreama和*ftstreamb
int setup(int argc, char **argv, char **fstreama, char **fstreamb, int *dim);
// 每个线程的工作
void *worker(void *arg);
// 计时函数
double wtime();

int main(int argc, char **argv)
{

    int n1, n2, n3;    // 矩阵A、B、C尺寸分别为n1*n2, n2*n3, n1*n3
    MPI_Status status; // 接收MPI_Recv的状态的变量

    int numthreads;  // 线程个数
    pthread_t *tids; // 线程列表

    double *matA, *matB, *matC; // A、B、C三个矩阵的存储空间
    double *A_row, *C_row;      // 得到的A中一行和C中一行

    // MPI启动
    int myid, numprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    char *fstreama, *fstreamb, *fstreamc; // 存放从文件中读出的矩阵信息或者要写入的矩阵信息
    char *commBuffer;                     // 由于矩阵可能很大，缓冲区不够时多个进程同时进行通信操作容易产生死锁，这里从用户空间里抽调内存作为MPI_Bsend使用的缓冲区
    int commBufSize;                      // 通信缓冲区大小
    int dim[3];                           // n1, n2, n3存在里面
    double elapsed_time;                  // 计时变量
    FILE *fhc;                            // 输出文件指针
    int fsizec;                           // 输出流长度

    if (!myid) {
        if (setup(argc, argv, &fstreama, &fstreamb, dim)) {
            exit(-1); // 预处理函数来源于串行版本，只有0号进程有必要进行这个操作
        }
        // 保存矩阵尺寸
        n1 = dim[0];
        n2 = dim[1];
        n3 = dim[2];
    }

    // 将读出的矩阵尺寸信息广播到其他进程
    MPI_Bcast(&n1, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n2, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n3, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (!myid) {
        // 准备输出矩阵C需要的条件
        fsizec = sizeof(int) * 2 + sizeof(double) * n1 * n3;
        if (!(fhc = fopen(argv[3], "w"))) {
            printf("Can't open file %s, Errno=%d\n", argv[3], errno);
            exit(-1);
        }
        // 为矩阵C的输出流开辟空间
        fstreamc = (char *)malloc(fsizec);
        // 输出流前两位是矩阵尺寸。
        ((int *)fstreamc)[0] = n1;
        ((int *)fstreamc)[1] = n3;
        // 准备好参与运算的矩阵，前两位int代表矩阵尺寸所以预先偏移指针
        matA = (double *)(fstreama + sizeof(int) * 2);
        matB = (double *)(fstreamb + sizeof(int) * 2);
        matC = (double *)(fstreamc + sizeof(int) * 2);
        // 计时
        elapsed_time = wtime();
    } else {
        // 与此同时其他进程开始为后面要从0号进程接收的矩阵B预留空间
        // 这必须在广播得知矩阵B大小尺寸后才能完成
        // 这样安排可以提高处理器利用率，节约一些运行时间
        matB = (double *)malloc(sizeof(double) * n2 * n3);
        // 考虑到0号进程负载明显更高，把另外两个分配空间的操作也前移
        A_row = (double *)malloc(n2 * sizeof(double));
        C_row = (double *)malloc(n3 * sizeof(double));
    }
    // 等到0号进程之外所有进程都准备好了矩阵B的空间才能广播
    MPI_Barrier(MPI_COMM_WORLD);
    // 向所有进程广播矩阵B
    MPI_Bcast(matB, n2 * n3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    commBufSize = sizeof(double) * (n3 + n2) + BUFSIZ; // 最坏情况是所有进程同时都在发送，这里开一个比这一操作需要的缓冲区更大的缓冲区
    if ((commBuffer = (char *)malloc(commBufSize)) == NULL) {
        // 这一操作的空间复杂度是很大的
        printf("Not enough memory!\n");
        return -1;
    }
    MPI_Buffer_attach(commBuffer, commBufSize); // 将缓冲区交给MPI管理
    if (!myid) {
        int j, numsend, sender;
        // 分配矩阵A到各个进程
        j = (numprocs - 1) < n1 ? (numprocs - 1) : n1;
        for (int i = 1; i <= j; i++) {
            MPI_Bsend(matA + (i - 1) * n2, n2, MPI_DOUBLE, i, 99, MPI_COMM_WORLD);
        }
        // 逐个接收计算结果
        numsend = j;
        for (int i = 1; i <= n1; i++) {
            sender = (i - 1) % (numprocs - 1) + 1;
            MPI_Recv(matC + (i - 1) * n3, n3, MPI_DOUBLE, sender, 100, MPI_COMM_WORLD, &status);
            // 解决之前没有送出的一部分
            if (numsend < n1) {
                // 这里从之前送到的矩阵行之后开始送
                // 原版代码在这里是有问题的，相当于从头开始送数据了
                MPI_Bsend(matA + numsend * n2, n2, MPI_DOUBLE, sender, 99, MPI_COMM_WORLD);
                numsend++;
            } else {
                // 送完了就不用再送了
                // 但是其他进程并不能确定送没送完
                // 所以这里用一个标记为0的消息告诉接到消息的线程工作已经完成不用再干了
                MPI_Bsend(&numsend, 0, MPI_INT, sender, 0, MPI_COMM_WORLD);
            }
        }

    } else {
        // 确定每个进程可用线程数
        numthreads = get_nprocs();
        // 存储线程id的空间
        tids = (pthread_t *)malloc(numthreads * sizeof(pthread_t));
        // 为每个线程准备一个线程参数结构体
        // 每个线程的结果先回到自己对应的数组元素
        // 这也起到了临界区保护的作用
        struct threadArg *targs = (struct threadArg *)malloc(numthreads * sizeof(struct threadArg));
        for (int i = 0; i < numthreads; i++) {
            targs[i].tid = i;
            targs[i].B = matB;
            targs[i].A_row = A_row;
            targs[i].C_row = C_row;
            targs[i].numthreads = numthreads;
            targs[i].N = n2;
            targs[i].P = n3;
        }
        while (1) {
            // 各进程从0号线程处接受矩阵A的一行
            MPI_Recv(A_row, n2, MPI_DOUBLE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            // 如果接到的是标志计算已经完成的消息就不用算了
            if (status.MPI_TAG == 0) {
                // 释放资源
                free(tids);
                free(targs);
                free(matB);
                free(A_row);
                free(C_row);
                break;
            }
            // 开始计算
            for (int i = 0; i < numthreads; ++i) {
                pthread_create(&tids[i], NULL, worker, &targs[i]);
            }
            // 算完了回收资源
            // 计算结果都是指针传递，所以不用考虑返回值之类的问题
            for (int i = 0; i < numthreads; ++i) {
                pthread_join(tids[i], NULL);
            }
            // 把结果发回给0号
            MPI_Bsend(C_row, n3, MPI_DOUBLE, 0, 100, MPI_COMM_WORLD);
        }
    }
    // 确保所有进程都发送完消息了再释放通信缓冲区
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Buffer_detach(commBuffer, &commBufSize);
    free(commBuffer);
    if (!myid) {
        // 收尾工作，计时、释放资源
        elapsed_time = wtime() - elapsed_time;

        printf("Parallel algrithm: multiply a %dx%d with a %dx%d, use %.2lf(s)\n",
               n1, n2, n2, n3, elapsed_time);

        fwrite(fstreamc, sizeof(char), fsizec, fhc);
        fclose(fhc);
        free(fstreama);
        free(fstreamb);
        free(fstreamc);
    }
    MPI_Finalize();
    return 0;
}

int setup(int argc, char **argv, char **fstreama, char **fstreamb, int *dim)
{
    // 这个函数是从串行版本复制得到的
    // 命令行参数判断
    if (argc < 4) {
        printf("Invalid arguments!\n");
        printf("Usage: ./serial filea fileb filec\n");
        printf("filea, fileb and filec are file names for matrix A, B and C\n");
        return 1;
    }

    FILE *fha, *fhb;
    int fsizea, fsizeb;
    // 打开文件
    if (!(fha = fopen(argv[1], "r"))) {
        printf("Can't open matrix file %s, Errno=%d\n", argv[1], errno);
        return 1;
    }

    if (!(fhb = fopen(argv[2], "r"))) {
        printf("Can't open file %s, Errno=%d\n", argv[2], errno);
        return 1;
    }
    // 存储文件信息
    struct stat fstata, fstatb;
    stat(argv[1], &fstata);
    stat(argv[2], &fstatb);
    fsizea = fstata.st_size;
    fsizeb = fstatb.st_size;
    // 将文件信息存入内存
    *fstreama = (char *)malloc(fsizea);
    *fstreamb = (char *)malloc(fsizeb);
    fread(*fstreama, sizeof(char), fsizea, fha);
    fread(*fstreamb, sizeof(char), fsizeb, fhb);
    // 得到矩阵尺寸
    int n1, n2, n3, n4;
    n1 = ((int *)(*fstreama))[0];
    n2 = ((int *)(*fstreama))[1];
    n3 = ((int *)(*fstreamb))[0];
    n4 = ((int *)(*fstreamb))[1];
    // 校验读入矩阵的合法性
    if (n1 <= 0 || n2 <= 0 || n3 <= 0 || n4 <= 0 || n2 != n3) {
        printf("Matrix size error, %dx%d with %dx%d\n", n1, n2, n3, n4);
        return 1;
    }

    if (fsizea < (sizeof(int) * 2 + sizeof(double) * n1 * n2)) {
        printf("Actual size of A mismatches with stated size\n");
        return 1;
    }

    if (fsizeb < (sizeof(int) * 2 + sizeof(double) * n3 * n4)) {
        printf("Actual size of B mismatches with stated size\n");
        return 1;
    }
    // 存好尺寸传出
    dim[0] = n1;
    dim[1] = n2;
    dim[2] = n4;
    // 收尾工作
    fclose(fha);
    fclose(fhb);
    return 0;
}

void *worker(void *arg)
{
    int i, j;
    struct threadArg *myarg = (struct threadArg *)arg;
    for (i = myarg->tid; i < myarg->P; i += myarg->numthreads) {
        // 平均分配B的所有列
        myarg->C_row[i] = 0.0;
        for (j = 0; j < myarg->N; j++) {
            // B中的一列与A行相乘，计算结果存入C一行中的对应位置，也即按定义计算矩阵乘法
            myarg->C_row[i] += myarg->A_row[j] * *(myarg->B + j * myarg->P + i);
        }
    }
    return NULL;
}

double wtime()
{
    // 从串行版本中挪来的计时函数
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}