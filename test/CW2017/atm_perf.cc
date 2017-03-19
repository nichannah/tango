
#include <mpi.h>
#include <vector>
#include "tango.h"

#define NUM_FIELDS 1
#define NUM_TIMESTEPS 100

using namespace std;

int main(int argc, char* argv[])
{
    int g_rows = 1080, g_cols = 1440, l_rows = 1080, l_cols = 1440;
    vector<double *> fields;

    for(int f = 0; f < NUM_FIELDS; f++) {
        fields.push_back(new double[l_rows * l_cols]);
    }

    MPI_Init(&argc, &argv);
    tango_init("./", "ice", 0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);

    for (size_t t = 0; t < NUM_TIMESTEPS; t++) {
        tango_begin_transfer("timestamp", "atm");

        for (size_t f = 0; f < NUM_FIELDS; f++) {
            tango_get("T_10", fields[f], l_rows * l_cols);
        }

        tango_end_transfer();
    }
    tango_finalize();
    MPI_Finalize();

    for (int f = 0; f < NUM_FIELDS; f++) {
        delete fields[f];
    }
}
