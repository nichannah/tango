#include <mpi.h>

extern "C" int call_mpi_comm_rank(void)
{
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    return rank;
}
