#pragma once

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <string>
#include <list>
#include <vector>

using namespace std;

class Config
{
private:
    /* Path to directory containing config.yaml and remapping weights files. */
    string config_dir;
    /* The grid that this process is on. */
    string local_grid_name;
    /* A peer grid, doesn't matter which one. */
    string a_peer_grid;

    /* Some information about the local grid. */
    /* File where the info was read. */
    string grid_info_file;
    unsigned int local_grid_size;

    /* Grids that we send to, receive from. */
    unordered_set<string> send_grids;
    unordered_set<string> recv_grids;

    /* Mapping from grid name to list of fields that can be sent/received
     * to/from this grid.  This is used for runtime checking. */
    unordered_map<string, list<string> > send_grid_to_fields_map;
    /* Read this as: the variables that we receive from each grid. */
    unordered_map<string, list<string> > recv_grid_to_fields_map;

public:
    Config(string config_dir, string grid_name)
        : config_dir(config_dir), local_grid_name(grid_name) {}
    void parse_config(void);
    void read_grid_info(void);
    string get_local_grid(void) const { return local_grid_name; }
    unsigned int get_local_grid_size(void) const { return local_grid_size; }
    string get_grid_info_file(void) const { return grid_info_file; }
    bool can_send_field_to_grid(string field, string grid);
    bool can_recv_field_from_grid(string field, string grid);
    void read_weights(string src_grid, string dest_grid,
                       vector<unsigned int>& src_points,
                       vector<unsigned int>& dest_points,
                       vector<double>& weights, bool sort_src) const;
    const unordered_set<string>& get_send_grids(void) const { return send_grids; }
    const unordered_set<string>& get_recv_grids(void) const { return recv_grids; }
    bool is_peer_grid(string grid) const;
    bool is_send_grid(string grid) const;
    bool is_recv_grid(string grid) const;
};

