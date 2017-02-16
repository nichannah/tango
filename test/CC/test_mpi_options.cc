
#include <mpi.h>
#include "gtest/gtest.h"

/* Test various approaches to MPI comms for sending coupling fields. */

/* Test alltoall() with 0 message sizes. */
TEST(MPI_Options, alltoall)
{
    int size;

    MPI_Init();

    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int sendbuf[size];
    int recvbuf[size];

    MPI_Alltoall(sendbuf, 0, MPI_INT, recvbuf, 1, MPI_INT, MPI_COMM_WORLD);
}

int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}


