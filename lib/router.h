#if !defined ROUTER_H
#define ROUTER_H

#include <map>
#include <list>
#include <vector>

using namespace std;

typedef unsigned int point_t;
typedef int tile_id_t;

/* A per-rank tile is a subdomain of a particular grid. */
class Tile {
public:

    /* Id of the tile is the MPI_COMM_WORLD rank on which the tile exists. */
    tile_id_t id;

    /* i, j extent of domain this tile contains. */
    unsigned int lis, lie, ljs, lje;
    /* A different representation of the above. It is a 1-D array of global
     * indices. e.g. on a 2x2 grid the indices would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points. */
    vector<point_t> points;

    /* Global extent domain that this tile is a part of. */
    unsigned int gis, gie, gjs, gje;

    /* Local points that need to be sent to this tile. */
    vector<point_t> send_points;
    /* Local points that will be received from this tile. FIXME: is this
     * any different from above? */
    vector<point_t> recv_points;
    /* Corrosponding weights for the points above. */
    vector<double> weights;


    Tile(tile_id_t id, int lis, int lie, int ljs, int lje,
         int gis, int gie, int gjs, int gje);
    bool has_point(point_t p) const;
    const vector<point_t>& get_send_points(void) const { return send_points; }
    const vector<double>& get_weights(void) const { return weights; }
    bool has_send_points(void) const { return !send_points.empty(); }
    point_t global_to_local(point_t global);
};

class Grid {
private:
    string name;

public:

    /* A list of tiles on this grid. These are only the tiles that this proc
     * needs to commincate with. */
    list<Tile *> tiles;
    Grid(string grid_name) : name(grid_name) {}
    ~Grid() { assert(false); }
};

class Router {
private:

    string local_grid_name;
    Tile *local_tile;

    int num_ranks;

    /* A list of remote grids that we communicate with. */
    map<string, Grid*> dest_grids;
    map<string, Grid*> src_grids;

    void remove_unreferenced_tiles(list<Tile *>& to_clean);
    void read_netcdf(string filename, vector<int>& src_points,
                     vector<int>& dest_points, vector<double>& weigts);
    bool is_dest_grid(string grid);
    bool is_src_grid(string grid);

public:
    Router(string grid_name, list<string>& dest_grids, list<string>& src_grids,
           int lis, int lie, int ljs, int lje,
           int gis, int gie, int gjs, int gje);
    ~Router() { assert(false); }
    void build_routing_rules(void);
    void exchange_descriptions(void);
    void build_rules(void);
    string get_local_grid_name(void) { return local_grid_name; }
    list<Tile *>& get_dest_tiles(string grid) { return dest_grids[grid]->tiles; }
    list<Tile *>& get_src_tiles(string grid) { return src_grids[grid]->tiles; }
};

/* A per-process coupling manager. */
class CouplingManager {
private:
    /* Router that responsible for arranging communications with other
     * processes. */
    Router *router;

    /* Mapping from grid name to list of field send/received to/from this grid.
     * This is used for runtime checking. */
    map<string, list<string> > dest_grid_to_fields;
    /* Read this as: the variables that we receive from each grid. */
    map<string, list<string> > src_grid_to_fields;

    void parse_config(string config, list<string>& dest_grids,
                      list<string>& src_grids);
public:
    CouplingManager(string config, string grid_name,
                    int lis, int lie, int ljs, int lje,
                    int gis, int gie, int gjs, int gje);
    Router* get_router(void) { return router; }
    bool can_send_field_to_grid(string field, string grid);
    bool can_recv_field_from_grid(string field, string grid);
};


#endif /* ROUTER_H */
