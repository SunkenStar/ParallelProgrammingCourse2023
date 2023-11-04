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
    char message[100] = "Message has returned"; // Message to be sent
    double start, end; // Start and end for timing
    l = strlen(message) + 1;
    // Initialize MPI, set myid and numprocs
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // Fix the message content, and use no complex calculation to get source and destination
    if (myid == 0) {
        // Process 0 send the message to process 1 and wait for process N to send back
        // The order is MPI_Send first, then MPI_Recv
        start = MPI_Wtime();
        MPI_Send(message, l, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        // Process waiting for returned message
        MPI_Recv(message, 100, MPI_CHAR, numprocs - 1, 0, MPI_COMM_WORLD, &status);
        // Calculate the time between sending from Process 0 and receiving of Process 0
        // This is the time of the communication Ring.
        end = MPI_Wtime();
        printf("%s\n", message);
        printf("%lf\n", end - start);
    } else if (myid == numprocs - 1){
        // The communication order is Receive first and then Send
        // No dead lock in this order even the buffer size is limitted
        source = myid - 1;
        dest = 0;
        MPI_Recv(message, 100, MPI_CHAR, source, 0, MPI_COMM_WORLD, &status);
        MPI_Send(message, l, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
    } else {
        source = myid - 1;
        dest = myid + 1;
        MPI_Recv(message, 100, MPI_CHAR, source, 0, MPI_COMM_WORLD, &status);
        MPI_Send(message, l, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
    }
    // Release resources
    MPI_Finalize();
    return 0;
}