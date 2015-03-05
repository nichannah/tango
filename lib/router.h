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
private:

    Tile *remote_tile

    /* These two maps make up a symetrical graph structure. Each one maps
     * individial points to a list of peer points with an associated weight.
     * The peer points from one map are the keys to the other.
     *
     * It is not necessary to have both maps since they represent the same
     * relationship between points, however keeping two is convenient.
     *
     * The local_side_map uses local points has keys, the other uses remote
     * points as keys.
     */
    map<point_t, list< pair<point_t, weight_t> > local_side_map;
    map<point_t, list< pair<point_t, weight_t> > remote_side_map;
public:
    Mapping(Tile *remote_tile) : remote_tile(remote_tile) {}
};

class Router {
private:

    string local_grid_name;
    Tile *local_tile = nullptr;

    int num_ranks;

    /* Mappings that the local_tile participates in. */
    unordered_map<string, list<Mapping *> > send_mapping;
    unordered_map<string, list<Mapping *> > recv_mapping;

    /* Map grid names to lists of tiles. FIXME: is this needed? */
    unordered_map<string, list<Tile *> > grid_tiles;

    /* Grids that we send to and recv from. */
    unordered_set<string> send_grids;
    unordered_set<string> recv_grids;

    void remove_unreferenced_tiles(void);
    void read_netcdf(string filename, vector<unsigned int>& send_points,
                     vector<unsigned int>& recv_points,
                     vector<double>& weights);
    bool is_peer_grid(string grid);
    bool is_send_grid(string grid);
    bool is_recv_grid(string grid);

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

    void add_to_send_mapping(string grid, unsigned int src_point,
                             unsigned int dest_point, double weight);
    void add_to_recv_mapping(string grid, unsigned int src_point,
                             unsigned int dest_point, double weight);
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
