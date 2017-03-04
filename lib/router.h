#pragma once

#include <unordered_map>
#include <set>
#include <list>
#include <vector>
#include <algorithm>

#include "config.h"

using namespace std;

typedef unsigned int point_t;
typedef double weight_t;
typedef int tile_id_t;

/* A per-rank tile represents a subdomain of a particular grid. */
class Tile {
private:

    /* Id of the tile is the MPI_COMM_WORLD rank on which the tile exists. */
    tile_id_t id;

    /* i, j extent of domain this tile contains. */
    unsigned int lis, lie, ljs, lje;
    /* A different representation of the above. It is a 1-D array of global
     * indices. e.g. on a 2x2 grid the indices would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points.
     * This is kept sorted for performance reasons. */
    vector<point_t> points;

    /* Global extent domain that this tile is a part of. */
    unsigned int gis, gie, gjs, gje;

public:
    Tile(tile_id_t tile_id, int lis, int lie, int ljs, int lje,
         int gis, int gie, int gjs, int gje);
    point_t global_to_local_domain(point_t global) const;
    const vector<point_t>& get_points(void) const { return points; }
    bool domain_equal(const shared_ptr<Tile>& another_tile) const;
    tile_id_t get_id(void) const { return id; }
    bool has_point(point_t p) const
        {
            /* Since points is sorted we can do this. */
            return binary_search(points.begin(), points.end(), p);
        }
    void pack(int *box, size_t size);
};

/* This represents a mapping between the local tile (proc) to a remote tile in
 * _either_ a send or receive direction (not both). The send and receive are
 * kept separate because there may be different interpolation schemes used for
 * each. */
class Mapping {
private:

    shared_ptr<Tile> remote_tile;

    /* Ordered set of 'side A' points in the mapping. They are the keys to the map
     * below. It's a convenience and also provides necessary ordering. */
    set<point_t> side_A_points;

    /* This map represents a graph structure. It maps individial 'side A'
     * points to a list of peer points ('side B' points) with an associated
     * weight. The ordering of the side B points is just the order in which the
     * weights get applied. The ordering is only for numerical consistency. */
    unordered_map<point_t, set< pair<point_t, weight_t> > > side_A_to_B_map;

public:
    Mapping(shared_ptr<Tile> remote_tile) : remote_tile(remote_tile) {}
    void add_link(point_t side_A_point, point_t side_B_point, weight_t weight)
        {
            side_A_points.insert(side_A_point);
            side_A_to_B_map[side_A_point].insert(make_pair(side_B_point, weight));
        }
    const shared_ptr<Tile>&  get_remote_tile(void) const { return remote_tile; }

    const set<point_t>& get_side_A_points(void) const { return side_A_points; }
    const set< pair<point_t, weight_t> >& get_side_B(point_t p) const
        {
            auto it = side_A_to_B_map.find(p);
            assert(it != side_A_to_B_map.end());
            return it->second;
        }

    bool not_in_use(void) const { return side_A_points.empty(); }  
    tile_id_t get_remote_tile_id(void) const { return remote_tile->get_id(); }
};

class Router {
private:

    unique_ptr<Tile> local_tile;
    const Config& config;

    int num_ranks;

    /* Mappings that the local_tile participates in. */
    unordered_map<string, list<shared_ptr<Mapping> > > send_mappings;
    unordered_map<string, list<shared_ptr<Mapping> > > recv_mappings;

    void remove_unused_mappings(void);
    bool is_peer_grid(string grid);
    bool is_send_grid(string grid);
    bool is_recv_grid(string grid);

    void add_link_to_send_mapping(string grid, point_t src_point,
                                  point_t dest_point, weight_t weight);
    void add_link_to_recv_mapping(string grid, point_t src_point,
                                  point_t dest_point, weight_t weight);

    void create_send_mapping(string grid, shared_ptr<Tile> t);
    void create_recv_mapping(string grid, shared_ptr<Tile> t);

public:
    Router(const Config& config,
           unsigned int lis, unsigned int lie, unsigned int ljs,
           unsigned int lje, unsigned int gis, unsigned int gie,
           unsigned int gjs, unsigned int gje);
    void build_routing_rules(void);
    void exchange_descriptions(void);
    int get_tile_id(void) const
        { assert(local_tile != nullptr); return local_tile->get_id(); }
    const list<shared_ptr<Mapping> >& get_send_mappings(string grid) const
        {
            auto v = send_mappings.find(grid);
            assert(v != send_mappings.end());
            return v->second;
        }
    const list<shared_ptr<Mapping> >& get_recv_mappings(string grid) const
        {
            auto v = recv_mappings.find(grid);
            assert(v != recv_mappings.end());
            return v->second;
        }
};
