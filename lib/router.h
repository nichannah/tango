#if !defined TANGO_INTERNAL_H
#define TANGO_INTERNAL_H

#include <map>
#include <list>
#include <set>

using namespace std;

typedef unsigned int point_t
typedef int tile_id_t
typedef double weight_t

/* A per-process coupling manager. */
class CouplingManager {
private:
    /* Router that responsible for arranging communications with other
     * processes. */
    Router router;

    /* Mapping from grid name to list of field send/received to/from this grid.
     * This is used for runtime checking. */
    map<string, list<string> > dest_grid_to_fields;
    /* Read this as: the variables that we receive from each grid. */
    map<string, list<string> > src_grid_to_fields;

    void parse_config(void);
public:
    CouplingManager(string grid_name, int lis, int lie, int ljs, int lje,
                                      int gis, int gie, int gjs, int gje);
    const Router& get_router(void) { return router; }
}

class Router {
private:

    string local_grid_name;
    Tile local_tile;

    /* A list of remote grids that we communicate with. */
    map<string, Grid> dest_grids;
    map<string, Grid> src_grids;

    void broadcast_descriptions(void);
    void remove_unreferenced_tiles(list<Tile *> &to_clean);
    bool is_dest_grid(string grid);
    bool is_src_grid(string grid);

public:
    Router(void);
    const list<Tile *>& get_dest_tiles(string grid) { return dest_grids[grid].tiles; }
    void build_rules(void);
};

class Grid {
public:
    string name;

    /* A list of tiles on this grid. These are only the tiles that this proc
     * needs to commincate with. */
    list<Tile *> tiles;
};

/* A per-rank tile is a subdomain of a particular grid. */
class Tile {
private:

    /* Id of the tile is the MPI_COMM_WORLD rank on which the tile exists. */
    tile_id id;

    /* i, j extent of domain this tile contains. */
    unsigned int lis, lie, ljs, lje;
    /* A different representation of the above. It is a 1-D array of global
     * indices. e.g. on a 2x2 grid the indices would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points. */
    vector<point> points;

    /* Global extent domain that this tile is a part of. */
    unsigned int gis, gie, gjs, gje;

    /* Local points that need to be sent to this tile. */
    vector<point> send_points;
    /* Local points that will be received from this tile. FIXME: is this
     * any different from above? */
    vector<point> recv_points;
    /* Corrosponding weights for the points above. */
    vector<weight> weights;

public:
    Tile(unsigned int description);
    bool has_point(p);
    const vector<point>& get_send_points(void) { return send_points; }
    const vector<point>& get_weights(void) { return weights; }
    bool has_send_points(void) { return !send_points.empty() };
};


#endif /* TANGO_INTERNAL_H */
