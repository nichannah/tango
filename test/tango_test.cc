
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
    /*
    int src_len = 50, dest_len = 100;
    */
    int src_len = 4, dest_len = 8;

    string config_dir = "./test_input-1_mappings-2_grids-4x4_to_8x8/";

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (rank == 0) {
        double send_array[src_len * src_len];
        for (int i = 0; i < src_len * src_len; i++) {
            send_array[i] = i;
        }

        tango_init(config_dir.c_str(), "ice",
                   0, src_len, 0, src_len, 0, src_len, 0, src_len);
        tango_begin_transfer(0, "ocean");
        tango_put("temp", send_array, src_len * src_len);
        tango_end_transfer();

    } else {
        double recv_array[dest_len * dest_len];

        tango_init(config_dir.c_str(), "ocean",
                   0, dest_len, 0, dest_len, 0, dest_len, 0, dest_len);
        tango_begin_transfer(0, "ice");
        tango_get("temp", recv_array, dest_len * dest_len);
        tango_end_transfer();

        double expected_sum = 0;
        for (int i = 0; i < src_len * src_len; i++) {
            expected_sum += i;
        }

        /* Calculate sum of recv_array. */
        double area_ratio = double(src_len * src_len) /
                            double(dest_len * dest_len);
        double sum = 0;
        for (int i = 0; i < dest_len * dest_len; i++) {
            sum += recv_array[i] * area_ratio;
        }
        EXPECT_NEAR(expected_sum, sum, 1e-6);
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
