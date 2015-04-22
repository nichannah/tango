#include <Python.h>
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int flag = 0;

    printf("Using custom Python interpreter at %s\n", argv[0]);

    MPI_Initialized(&flag);
    if (!flag) {
        MPI_Init(&argc, &argv);
    }

    try {
        Py_Initialize();
        Py_Main(argc, argv);
        Py_Finalize();
    } catch ( ... ) {
        MPI_Finalize();
    }

    MPI_Finalize();
}
