
#include "gtest/gtest.h"
#include "router.h"

using namespace std;

TEST(TileTest, create_base)
{
    tile_id_t tile_id = 0;
    int g_rows = 4, g_cols = 4;
    Tile tile(tile_id, 0, g_rows, 0, g_cols, 0, g_rows, 0, g_cols);

    EXPECT_EQ(tile.points.size(), g_rows*g_cols);
    EXPECT_EQ(tile.points[0], 1);
    EXPECT_EQ(tile.points[g_rows*g_cols - 1], g_rows*g_cols);
}

TEST(TileTest, create_simple)
{
    tile_id_t tile_id = 0;
    int g_rows = 4, g_cols = 4;
    Tile tile(tile_id, 1, 3, 1, 2, 0, g_rows, 0, g_cols);

    EXPECT_EQ(tile.points.size(), 2);
    EXPECT_EQ(tile.points[0], 6);
    EXPECT_EQ(tile.points[1], 10);
}

TEST(TileTest, create_tricky)
{
    tile_id_t tile_id = 0;
    int g_rows = 100, g_cols = 100;
    Tile tile(tile_id, 10, 20, 10, 20, 0, g_rows, 0, g_cols);

    EXPECT_EQ(tile.points.size(), 100);
    EXPECT_EQ(tile.points[0], (10 * g_cols) + 10 + 1);
}

int main(int argc, char* argv[])
{
    int result = 0;

    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    return result;
}
