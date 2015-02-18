#if !defined ROUTER_H
#define ROUTER_H

#include <map>
#include <vector>
#include <set>

using namespace std;

/* FIXME: split out into a separate Grid class? */

class Router {
private:
    int my_rank;
    string my_name;
    int num_procs;
    unsigned int grid_id;
    unsigned int gis, gie, gjs, gje, lis, lie, ljs, lje;
    set<unsigned int> grid_points;

    /* The routing information is kept in a collection of maps. */

    /* The key is a grid, the value is a list of variables which is
     * sent/received. */
    /* Read this as: the variables that we send to each grid. */
    map<string, vector<string> > send_grid_to_vars_map;
    /* Read this as: the variables that we receive from each grid. */
    map<string, vector<string> > recv_grid_to_vars_map;

    /* Each key is a variable, the value is a list/vector of peer ranks to
     * contact when sending/receiveing this variable. */
    map<string, vector<unsigned int> > send_map;
    map<string, vector<unsigned int> > recv_map;

    /* The key is a rank id, the value is a list of indices into the domain. */
    map<int, vector<unsigned int> > send_rank_to_domain_map;
    map<int, vector<unsigned int> > recv_rank_to_domain_map;

    void parse_config(void);
    void broadcast_descriptions(void);
public:
    Router(string name, int isd, int ied, int jsd, int jed);
    void build_rules(void);
};

#endif /* ROUTER_H */
