
#include <algorithm>

#include "mpi.h"
#include "gtest/gtest.h"
#include "router.h"

using namespace std;

/* Single tile tests. */

TEST(Router, exchange_descriptions)
{
    int rank;
    int g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4;
    unordered_set<string> dest_grids, src_grids;

    string config_dir = "./test_input-2_mappings-2_grids/";
    string local_grid, peer_grid;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        local_grid = "ocean";
        peer_grid = "ice";
    } else {
        local_grid = "ice";
        peer_grid = "ocean";
    }

    CouplingManager cm(config_dir, local_grid);
    cm.parse_config(config_dir, local_grid, dest_grids, src_grids);

    Router r(local_grid, dest_grids, src_grids,
             0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);

    EXPECT_EQ(r.get_local_grid_name(), local_grid);
    r.exchange_descriptions();

    list<Tile *>& grid_tiles = r.get_grid_tiles(peer_grid);
    EXPECT_EQ(grid_tiles.size(), 1);

    /* Check the details of this tile. */
    Tile *tile = grid_tiles.front();
    if (rank == 0) {
        EXPECT_EQ(tile->get_id(), 1);
    } else {
        EXPECT_EQ(rank, 1);
        EXPECT_EQ(tile->get_id(), 0);
    }

    for (int i = 1; i < (g_cols * g_rows) + 1; i++) {
        EXPECT_EQ(tile->has_point(i), true);
    }
}

TEST(Router, build_routing_rules)
{
    int rank;
    int g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4;
    unordered_set<string> dest_grids, src_grids;

    string config_dir = "./test_input-2_mappings-2_grids/";
    string local_grid, peer_grid;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        local_grid = "ocean";
        peer_grid = "ice";
    } else {
        local_grid = "ice";
        peer_grid = "ocean";
    }

    CouplingManager cm(config_dir, local_grid);
    cm.build_router(0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);
    Router *router = cm.get_router();

    list<Tile *>& grid_tiles = router->get_grid_tiles(peer_grid);

    /* Since the local domain covers the entire global domain we expect only
     * one tile. */
    EXPECT_EQ(grid_tiles.size(), 1);

    Tile *tile = grid_tiles.front();
    EXPECT_EQ(tile->get_send_points().size(), l_cols*l_rows);
    EXPECT_EQ(tile->get_recv_points().size(), l_cols*l_rows);
    auto recv_points = tile->get_recv_points();
    auto send_points = tile->get_send_points();

    for (int i = 1; i < (l_cols * l_rows) + 1; i++) {
        auto itr = find(recv_points.begin(), recv_points.end(), i);
        EXPECT_NE(itr, recv_points.end());

        auto its = find(send_points.begin(), send_points.end(), i);
        EXPECT_NE(its, send_points.end());
    }
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
