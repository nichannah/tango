
#include <mpi.h>

#include "gtest/gtest.h"
#include "tango.h"

using namespace std;

/* Do single field send/receive between two grids/models. */
TEST(Tango, send_receive)
{
    int rank;
    int g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4;

    string config_dir = "./test_input-1_mappings-2_grids/";

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double send_sst[] = {292.1, 295.7, 290.5, 287.9,
                         291.3, 294.3, 291.8, 290.0,
                         292.1, 295.2, 290.8, 284.7,
                         293.3, 290.1, 297.8, 293.4 };
    assert(sizeof(send_sst) == (l_rows * l_cols * sizeof(double)));

    if (rank == 0) {
        tango_init(config_dir.c_str(), "ocean", 0, l_rows, 0, l_cols,
                                                0, g_rows, 0, g_cols);
        tango_begin_transfer(0, "ice");
        tango_put("sst", send_sst, l_rows * l_cols);
        tango_end_transfer();
        tango_finalize();

    } else {
        double recv_sst[l_rows * l_cols] = {};
        assert(sizeof(recv_sst) == (l_rows * l_cols * sizeof(double)));

        tango_init(config_dir.c_str(), "ice", 0, l_rows, 0, l_cols,
                                              0, g_rows, 0, g_cols);
        tango_begin_transfer(0, "ocean");
        tango_get("sst", recv_sst, l_rows * l_cols);
        tango_end_transfer();
        tango_finalize();

        /* Check that send and receive are the same. */

    }
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
