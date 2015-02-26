
#include <list>
#include <mpi.h>

#include "gtest/gtest.h"
#include "router.h"

using namespace std;

TEST(CouplingManagerTest, parse_config)
{
    list<string> dest_grids, src_grids;

    string config_dir = "./basic_test_input/";
    string grid = "ocean";

    CouplingManager cm(config_dir, grid);
    cm.parse_config(config_dir, grid, dest_grids, src_grids);

    EXPECT_EQ(dest_grids.front(), "ice");
    EXPECT_EQ(src_grids.front(), "ice");

    EXPECT_TRUE(cm.can_send_field_to_grid("sst", "ice"));
    EXPECT_TRUE(cm.can_recv_field_from_grid("temp", "ice"));
    EXPECT_TRUE(cm.can_recv_field_from_grid("swflux", "ice"));

    EXPECT_FALSE(cm.can_send_field_to_grid("sst", "atm"));
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
