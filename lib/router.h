#if !defined ROUTER_H
#define ROUTER_H

#include <map>
#include <list>
#include <set>

using namespace std;

typedef unsigned int point
typedef double weight
typedef grid string
typedef field string

class RemoteProc {
private:
    /* Represents a remote proc and the information needed to map data to it. */
    unsigned int rank;

    /* Grid that the remote proc is on. */
    string grid_name;
    unsigned int grid_id;

    /* i, j extent of local domain this proc is responsible for. */
    unsigned int lis, lie, ljs, lje;
    /* Global points that this proc is responsible for. */
    /* It is a 1-D array of global indices. e.g. on a 2x2 grid the indices
     * would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points. */
    vector<point> global_points;

    /* Local points that need to be sent to this remote proc. */
    vector<point> send_points;
    /* Corrosponding weights for the points above. */
    vector<weight> send_weights;
public:
    RemoteProc(unsigned int description);
    bool no_send_points(void) { return send_points.empty() };
    bool has_global_point(p); 
    const vector<point>& get_send_points(void);
    const vector<weight>& get_send_weights(void);
};


class Router {
private:

    /* The grid that this router works for. */
    /* FIXME: move these into a LocalProc object. */
    grid my_grid;
    unsigned int my_grid_id;
    int my_rank;
    int num_procs;

    /* i, j extent of local domain that router is repsonsible for. */
    unsigned int lis, lie, ljs, lje;
    /* Local domain as a 1-D array of global indices. e.g. on a
     * 2x2 grid the indices would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points. */
    vector<point> local_points;

    /* Global domain. */
    unsigned int gis, gie, gjs, gje;

    /* The key is a grid name, the value is a list of fields which is
     * sent/received. These are only needed for runtime checking. */
    /* Read this as: the fields that we send to each grid. */
    map<grid, list<field> > dest_grid_to_fields_map;
    /* Read this as: the variables that we receive from each grid. */
    map<grid, list<field> > src_grid_to_fields_map;

    /* Map of grid id's to grid names. This is global information determined by
     * the config file. */
    map<unsigned int, grid> grid_id_to_name_map;

    /* A list of grids that we communicate with. Easier to keep these around
     * than construct on the fly. */
    list<grid> dest_grids;
    list<grid> src_grids;

    /* Each key is a grid, value is a list of procs to contact when
     * sending/receiveing to/from this grid. Use a list because a lot of random
     * insert and remove. */
    map<grid, list<RemoteProc> > dest_map;
    map<grid, list<RemoteProc> > src_map;

    void parse_config(void);
    void broadcast_descriptions(void);

public:
    Router(string name, int isd, int ied, int jsd, int jed);
    void build_rules(void);

    set<RemoteProc>& get_dest_procs(string grid);
    set<RemoteProc>& get_src_procs(string grid);
};

#endif /* ROUTER_H */
