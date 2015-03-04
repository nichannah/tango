
#include <mpi.h>

#include "gtest/gtest.h"
#include "tango.h"

using namespace std;

/* Do single field send/receive between two grids/models. */
TEST(Tango, send_receive)
{
    int rank;
    int g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4;

    string config_dir = "./test_input-1_mappings-2_grids-4x4_to_4x4/";

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

    } else {
        double recv_sst[l_rows * l_cols] = {};
        assert(sizeof(recv_sst) == (l_rows * l_cols * sizeof(double)));

        tango_init(config_dir.c_str(), "ice", 0, l_rows, 0, l_cols,
                                              0, g_rows, 0, g_cols);
        tango_begin_transfer(0, "ocean");
        tango_get("sst", recv_sst, l_rows * l_cols);
        tango_end_transfer();

        /* Check that send and receive are the same. */
        for (int i = 0; i < l_rows * l_cols; i++) {
            EXPECT_EQ(send_sst[i], recv_sst[i]);
        }
    }

    tango_finalize();
}

/* Do single field send/receive between grids of different sizes. */
TEST(Tango, send_receive_different_grids)
{
    int rank;

    string config_dir = "./test_input-1_mappings-2_grids-4x4_to_8x8/";

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (rank == 0) {
        double send_array[4*4];
        for (int i = 0; i < 4*4; i++) {
            send_array[i] = 1;
        }

        tango_init(config_dir.c_str(), "ice", 0, 4, 0, 4, 0, 4, 0, 4);
        tango_begin_transfer(0, "ocean");
        tango_put("temp", send_array, 4 * 4);
        tango_end_transfer();

    } else {
        double recv_array[8*8];

        tango_init(config_dir.c_str(), "ocean", 0, 8, 0, 8, 0, 8, 0, 8);
        tango_begin_transfer(0, "ice");
        tango_get("temp", recv_array, 8 * 8);
        tango_end_transfer();

        /* Calculate sum of recv_array. */
        double sum = 0;
        for (int i = 0; i < 8*8; i++) {
            sum += recv_array[i];
        }

        cout << "recv_array sum is " << sum << endl;
    }

    tango_finalize();
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
