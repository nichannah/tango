#include <mpi.h>

extern "C" int call_mpi_comm_rank(void)
{
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    return rank;
}

extern "C" int call_mpi_comm_size(void)
{
    int size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);

    return size;
}

extern "C" int call_mpi_initialized(void)
{
    int initialized;

    MPI_Initialized(&initialized);

    return initialized;
}
