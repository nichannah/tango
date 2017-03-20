
#include <mpi.h>
#include <vector>
#include "tango.h"
#include <assert.h>

#define NUM_FIELDS 10
#define NUM_TIMESTEPS 10

using namespace std;

int main(int argc, char* argv[])
{
    int g_rows = 1080, g_cols = 1440;
    vector<double *> fields;

    int rank, size;
    int my_size, my_rank, my_root, my_row, my_col;
    int i_pes, j_pes, i_per_pe, j_per_pe;
    int lis, lie, ljs, lje;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Provide the number of PEs and root PE as an argument */
    assert(argc == 3);
    my_size = atoi(argv[1]);
    my_root = atoi(argv[2]);
    my_rank = rank - my_root;

    if (my_size == 1) {
        i_pes = 1;
        j_pes = 1;
    } else if (my_size == 2) {
        i_pes = 2;
        j_pes = 1;
    } else if (my_size == 4) {
        i_pes = 2;
        j_pes = 2;
    } else if (my_size == 12) {
        i_pes = 4;
        j_pes = 3;
    } else if (my_size == 16) {
        i_pes = 4;
        j_pes = 4;
    } else if (my_size == 480) {
        i_pes = 24;
        j_pes = 20;
    } else if (my_size == 960) {
        i_pes = 32;
        j_pes = 30;
    } else {
        assert(false);
    }

    my_row = int(my_rank / i_pes);
    my_col = my_rank % i_pes;

    i_per_pe = int(g_cols / i_pes);
    j_per_pe = int(g_rows / j_pes);

    lis = my_col*i_per_pe;
    lie = lis + i_per_pe;
    ljs = my_row*j_per_pe;
    lje = ljs + j_per_pe;

    cout << "my_rank : " << my_rank << endl;
    cout << "my_row : " << my_row << endl;
    cout << "my_col : " << my_col << endl;
    cout << "lis: " << lis << " lie: " << lie << " ljs: " << ljs << " lje: " << lje << endl;

    for (int f = 0; f < NUM_FIELDS; f++) {
        fields.push_back(new double[i_per_pe * j_per_pe]);
    }

    tango_init("./", "ice", lis, lie, ljs, lje, 0, g_rows, 0, g_cols);

    for (size_t t = 0; t < NUM_TIMESTEPS; t++) {
        tango_begin_transfer("timestamp", "atm");

        for (size_t f = 0; f < NUM_FIELDS; f++) {
            tango_get("T_10", fields[f], i_per_pe * j_per_pe);
        }

        tango_end_transfer();
    }
    tango_finalize();
    MPI_Finalize();

    for (int f = 0; f < NUM_FIELDS; f++) {
        delete fields[f];
    }
}
