
#include "mpi.h"
#include "gtest/gtest.h"
#include "router.h"

using namespace std;

TEST(Router, exchange_descriptions)
{
    int g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4;
    list<string> dest_grids, src_grids;
    string config_dir = "./";
    string grid = "ocean";

    CouplingManager cm(config_dir, grid);
    cm.parse_config(config_dir, grid, dest_grids, src_grids);

    Router r(grid, dest_grids, src_grids,
             0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);

    EXPECT_EQ(r.get_local_grid_name(), grid);
}

/* These tests need to be run with mpirun -n 2 router_test.exe */
int main(int argc, char* argv[]) 
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    MPI_Init(&argc, &argv);
    result = RUN_ALL_TESTS();
    MPI_Finalize();

    return result;
}
