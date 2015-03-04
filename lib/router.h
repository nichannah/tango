#if !defined ROUTER_H
#define ROUTER_H

#include <unordered_map>
#include <unordered_set>
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

    Tile(tile_id_t id, int lis, int lie, int ljs, int lje,
         int gis, int gie, int gjs, int gje);
    bool has_point(point_t p) const;
    unsigned int local_domain_size(void) const { return points.size(); }
    unsigned int global_domain_size(void) const
        { return (gie - gis) * (gje - gjs); }
    bool domain_equal(unsigned int lis, unsigned int lie, unsigned int ljs,
                      unsigned int lje, unsigned int gis, unsigned int gie,
                      unsigned int gjs, unsigned int gje);
    point_t global_to_local_domain(point_t global);
    tile_id_t get_id(void) const { return id; }
};

/* This represents a mapping between the local tile (proc) to a remote tile in
 * _either_ a send or receive direction (not both). The send and receive are
 * kept separate because there may be different interpolation schemes used for
 * each. */
class Mapping {
public:

    Tile *remote_tile

    /* Local points that must be sent/received to/from the remote tile. */
    vector<point_t> local_points;

    /* For each of the above there may be several remote points with an
     * associated weight. */
    unordered_map<point_t, list< pair<point_t, double> > remote_points;

    const vector<point_t>& get_send_points(void) const
        { return send_points; }
    const vector<point_t>& get_recv_points(void) const
        { return recv_points; }
    const vector<double>& get_send_weights(void) const { return send_weights; }
    const vector<double>& get_recv_weights(void) const { return recv_weights; }
    bool send_points_empty(void) const { return send_points.empty(); }
    bool recv_points_empty(void) const { return recv_points.empty(); }
};

class Router {
private:

    string local_grid_name;
    Tile *local_tile = nullptr;

    int num_ranks;

    list<Mapping *> send_mapping;
    list<Mapping *> recv_mapping;

    /* Map grid names to lists of tiles. */
    unordered_map<string, list<Tile *> > grid_tiles;
    /* Source (to receive from) and destination (to send to) grids. */
    unordered_set<string> src_grids;
    unordered_set<string> dest_grids;

    void remove_unreferenced_tiles(void);
    void read_netcdf(string filename, vector<unsigned int>& src_points,
                     vector<unsigned int>& dest_points,
                     vector<double>& weights);
    bool is_peer_grid(string grid);

public:
    Router(string grid_name, unordered_set<string>& dest_grids,
           unordered_set<string>& src_grids,
           unsigned int lis, unsigned int lie, unsigned int ljs,
           unsigned int lje, unsigned int gis, unsigned int gie,
           unsigned int gjs, unsigned int gje);
    ~Router();
    void build_routing_rules(string config_dir);
    void exchange_descriptions(void);
    void build_rules(void);
    int get_tile_id(void) const
        { assert(local_tile != nullptr); return local_tile->id; }
    list<Tile *>& get_grid_tiles(string grid);
    string get_local_grid_name(void) { return local_grid_name; }
};

/* A per-process coupling manager. */
class CouplingManager {
private:
    /* Router that responsible for arranging communications with other
     * processes. */
    Router *router = nullptr;

    /* Path to directory containing config.yaml and remapping weights files. */
    string config_dir;
    /* The grid that this process is on. */
    string grid_name;

    /* Mapping from grid name to list of field send/received to/from this grid.
     * This is used for runtime checking. */
    unordered_map<string, list<string> > dest_grid_to_fields;
    /* Read this as: the variables that we receive from each grid. */
    unordered_map<string, list<string> > src_grid_to_fields;

public:
    CouplingManager(string config_dir, string grid_name);
    ~CouplingManager() { delete router; router = nullptr; }
    void build_router(int lis, int lie, int ljs, int lje,
                     int gis, int gie, int gjs, int gje);
    void parse_config(string config_dir, string local_grid_name,
                      unordered_set<string>& dest_grids,
                      unordered_set<string>& src_grids);
    Router* get_router(void) { return router; }
    bool can_send_field_to_grid(string field, string grid);
    bool can_recv_field_from_grid(string field, string grid);
};


#endif /* ROUTER_H */
