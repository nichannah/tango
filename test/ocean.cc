
#include <stdlib.h>
#include <mpi.h>

#include "tango.h"

#define NUM_TIMESTEPS 50
#define SECS_PER_TIMESTEP 2

int main(int argc, char* argv[])
{
    double *air_temp;
    double *shortwave_flux;
    int i, time;

    air_temp = new double[10];
    shortwave_flux = double[10];

    MPI_Init(&argc, &argv);
    tango_init();

    time = 0;
    for (i = 0; i < NUM_TIMESTEPS; i++) {
        tango_begin_comm(time)
        tango_recv(air_temp)
        tango_recv(shortwave_flux)
        tango_end_comm()

        time += SECS_PER_TIMESTEP;
    }

    MPI_Finalize();
    tango_finalize();

    delete(air_temp);
    delete(shortwave_flux);
}
