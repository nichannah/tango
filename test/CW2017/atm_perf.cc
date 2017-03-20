
#include <mpi.h>
#include <vector>
#include "tango.h"

#define NUM_FIELDS 10
#define NUM_TIMESTEPS 10

using namespace std;

int main(int argc, char* argv[])
{
    int g_rows = 94, g_cols = 192, l_rows = 94, l_cols = 192;
    vector<double *> fields;

    for (int f = 0; f < NUM_FIELDS; f++) {
        fields.push_back(new double[l_rows * l_cols]);
    }

    MPI_Init(&argc, &argv);

    const clock_t begin_time = clock();
    tango_init("./", "atm", 0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);

    for (size_t t = 0; t < NUM_TIMESTEPS; t++) {
        cout << "Timestep: " << t << endl;
        tango_begin_transfer("timestamp", "ice");

        for (size_t f = 0; f < NUM_FIELDS; f++) {
            tango_put("T_10", fields[f], l_rows * l_cols);
        }

        tango_end_transfer();
    }
    tango_finalize();
    cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC << "s" << endl;

    MPI_Finalize();

    for (int f = 0; f < NUM_FIELDS; f++) {
        delete fields[f];
    }
}
