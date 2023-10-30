// Necessary Headers
#include "mpi.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int numprocs,      // Number of processes
        myid,          // Process id
        l,             // Message length
        source,        // Source for each communication
        dest;          // Destination for each communication
    MPI_Status status; // For receiving the status returned from MPI_Recv
    char message[100]; // Message to be sent
    double start, end; // Start and end for timing

    // Initialize MPI, set myid and numprocs
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if (myid == 0) {
        sprintf(message, "greetings from Process %d!", myid);
        l = strlen(message) + 1;
        start = MPI_Wtime();
        MPI_Send(message, l, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(message, 100, MPI_CHAR, numprocs - 1, numprocs - 1, MPI_COMM_WORLD, &status);
        end = MPI_Wtime();
        printf("Process %d received %s\n", myid, message);
        printf("%lf\n", end - start);
    } else {
        source = (myid + numprocs - 1) % numprocs;
        dest = (myid + 1) % numprocs;
        MPI_Recv(message, 100, MPI_CHAR, source, source, MPI_COMM_WORLD, &status);
        printf("Process %d received %s\n", myid, message);
        sprintf(message, "greetings from Process %d!", myid);
        l = strlen(message) + 1;
        MPI_Send(message, l, MPI_CHAR, dest, myid, MPI_COMM_WORLD);
    }
    // Release resources
    MPI_Finalize();
    return 0;
}