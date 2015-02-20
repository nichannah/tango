
#include <stdlib.h>
#include <mpi.h>

#include "tango.h"

#define NUM_TIMESTEP 100
#define SECS_PER_TIMESTEP 1

int main(int argc, char* argv[])
{
    double *air_temp;
    double *shortwave_flux;
    int i, time;

    air_temp = new double[10];
    shortwave_flux = new double[10];

    MPI_Init(&argc, &argv);
    tango_init();

    time = 0;
    for (i = 0; i < NUM_TIMESTEPS; i++) {

        tango_begin_transfer(time);
        tango_put(air_temp);
        tango_put(shortwave_flux);
        tango_end_transfer();

        time += SECS_PER_TIMESTEP;
    }

    MPI_Finalize();

    delete(air_temp);
    delete(shortwave_flux);
}
