#if !defined ROUTER_H
#define ROUTER_H

#include <map>
#include <list>
#include <vector>

using namespace std;

typedef unsigned int point_t;
typedef int tile_id_t;

/* A per-rank tile represents a subdomain of a particular grid. */
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

    /* Local points to send this tile. */
    vector<point_t> send_points;
    /* Corrosponding weights for the points above. */
    vector<double> send_weights;
    /* Local points to recv from this tile. */
    vector<point_t> recv_points;
    /* Keep the recv_weights around for now, although not needed because they
     * are applied on the send side. */
    vector<point_t> recv_weights;

    Tile(tile_id_t id, int lis, int lie, int ljs, int lje,
         int gis, int gie, int gjs, int gje);
    bool has_point(point_t p) const;
    const vector<point_t>& get_send_points(void) const
        { return send_points; }
    const vector<point_t>& get_recv_points(void) const
        { return recv_points; }
    bool send_points_empty(void) const { return send_points.empty(); }
    bool recv_points_empty(void) const { return recv_points.empty(); }
    const vector<double>& get_weights(void) const { return weights; }
    point_t global_to_local_domain(point_t global);
    tile_id_t get_id(void) const { return id; }
};

class Router {
private:

    string local_grid_name;
    Tile *local_tile;

    int num_ranks;

    /* Map grid names to lists of tiles. */
    map<string, list<Tile *> > grid_tiles;
    /* Source (to receive from) and destination (to send to) grids. */
    list<string> src_grids;
    list<string> dest_grids;

    void remove_unreferenced_tiles(void);
    void read_netcdf(string filename, vector<int>& src_points,
                     vector<int>& dest_points, vector<double>& weights);
    bool is_peer_grid(string grid);

public:
    Router(string grid_name, list<string>& dest_grids, list<string>& src_grids,
           int lis, int lie, int ljs, int lje,
           int gis, int gie, int gjs, int gje);
    ~Router();
    void build_routing_rules(string config_dir);
    void exchange_descriptions(void);
    void build_rules(void);
    string get_local_grid_name(void) { return local_grid_name; }
};

/* A per-process coupling manager. */
class CouplingManager {
private:
    /* Router that responsible for arranging communications with other
     * processes. */
    Router *router;

    /* Path to directory containing config.yaml and remapping weights files. */
    string config_dir;
    /* The grid that this process is on. */
    string grid;

    /* Mapping from grid name to list of field send/received to/from this grid.
     * This is used for runtime checking. */
    map<string, list<string> > dest_grid_to_fields;
    /* Read this as: the variables that we receive from each grid. */
    map<string, list<string> > src_grid_to_fields;

public:
    CouplingManager(string config_dir, string grid_name);
    void build_router(int lis, int lie, int ljs, int lje,
                         int gis, int gie, int gjs, int gje);
    void parse_config(string config_dir, string local_grid_name,
                      list<string>& dest_grids, list<string>& src_grids);
    Router* get_router(void) { return router; }
    bool can_send_field_to_grid(string field, string grid);
    bool can_recv_field_from_grid(string field, string grid);
};


#endif /* ROUTER_H */
