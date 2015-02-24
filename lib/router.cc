
#include <netcdf>
#include <mpi.h>
#include <assert.h>
#include <fstream>
#include <yaml-cpp/yaml.h>

#include "router.h"

using namespace netCDF;

#define MAX_GRID_NAME_SIZE 32
#define DESCRIPTION_SIZE (MAX_GRID_NAME_SIZE + 1 + 9)
#define WEIGHT_THRESHOLD 1e12

Tile::Tile(tile_id_t tile_id, int lis, int lie, int ljs, int lje,
           int gis, int gie, int gjs, int gje)
    : id(tile_id), lis(lis), lie(lie), ljs(ljs), lje(lje)
{
    /* Set up list of points that this tile is responsible for with a global
     * reference. */
    int n_cols = gje - gjs;
    int n_local_rows = lie - lis;
    int i_offset = lis - gis;
    int j_offset = ljs - gjs;

    for (int i = i_offset; i < n_local_rows; i++) {
        point_t index = (n_cols * i) + j_offset;

        for (int j = ljs; j < lje; j++) {
            points.push_back(index);
            index++;
        }
    }
}

bool Tile::has_point(point_t p) const
{
    for (auto point : points) {
        if (p == point) {
            return true;
        }
    }
    return false;
}

point_t Tile::global_to_local(point_t global)
{
    point_t local = 0;
    for (auto p : points) {
        if (p == global) {
            return local;
        }
        local++;
    }
    assert(false);
}

Router::Router(string grid_name,
               list<string>& dest_grid_names, list<string>& src_grid_names,
               int lis, int lie, int ljs, int lje,
               int gis, int gie, int gjs, int gje) : local_grid_name(grid_name)
{
    tile_id_t tile_id;

    MPI_Comm_rank(MPI_COMM_WORLD, &tile_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    local_tile = new Tile(tile_id, lis, lie, ljs, lje, gis, gie, gjs, gje);
    for (const auto& grid_name : dest_grid_names) {
        assert(dest_grids.find(grid_name) == dest_grids.end());
        dest_grids[grid_name] = new Grid(grid_name);
    }
    for (const auto& grid_name : src_grid_names) {
        assert(src_grids.find(grid_name) == src_grids.end());
        src_grids[grid_name] = new Grid(grid_name);
    }
}

/* Pack a description of this proc and broadcast it to all others. They'll use
 * the information to set up their routers. */
void Router::exchange_descriptions(void)
{
    unsigned int description[DESCRIPTION_SIZE];
    unsigned int *all_descs;
    int i;

    /* Marshall my description into an array. */
    /* We waste some bytes here when sending the grid name. */
    assert(local_grid_name.size() <= MAX_GRID_NAME_SIZE);
    for (i = 0; i < MAX_GRID_NAME_SIZE; i++) {
        if (i < local_grid_name.size()) {
            description[i] = local_grid_name[i];
            assert(description[i] != '\0');
        } else {
            description[i] = '\0';
        }
    }
    description[i++] = local_tile->id;
    description[i++] = local_tile->lis;
    description[i++] = local_tile->lie;
    description[i++] = local_tile->ljs;
    description[i++] = local_tile->lje;
    description[i++] = local_tile->gis;
    description[i++] = local_tile->gie;
    description[i++] = local_tile->gjs;
    description[i++] = local_tile->gje;

    /* Distribute all_descriptions. */
    all_descs = new unsigned int[DESCRIPTION_SIZE * num_ranks];
    MPI_Gather(description, DESCRIPTION_SIZE, MPI_UNSIGNED, all_descs,
               DESCRIPTION_SIZE * num_ranks, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(all_descs, DESCRIPTION_SIZE * num_ranks, MPI_UNSIGNED,
                  0, MPI_COMM_WORLD);

    /* Unmarshall into Tile objects. */
    for (int i = 0; i < num_ranks * DESCRIPTION_SIZE; i += DESCRIPTION_SIZE) {

        int j = i;
        string grid_name;

        for (; j < MAX_GRID_NAME_SIZE; j++) {
            if (all_descs[j] != '\0') {
                grid_name.push_back((char)all_descs[j]);
            }
        }

        /* Insert into maps. These will be refined later and unnecessary
         * tiles that we don't actually communicate with will be deleted.
         * */
        if (is_dest_grid(grid_name)) {
            /* A tile could get big, so we make pointers and avoid copying. */
            Tile *t = new Tile(all_descs[j++], all_descs[j++], all_descs[j++],
                               all_descs[j++], all_descs[j++], all_descs[j++],
                               all_descs[j++], all_descs[j++], all_descs[j++]);
            get_dest_tiles(grid_name).push_back(t);
        }
        if (is_src_grid(grid_name)) {
            Tile *t = new Tile(all_descs[j++], all_descs[j++], all_descs[j++],
                               all_descs[j++], all_descs[j++], all_descs[j++],
                               all_descs[j++], all_descs[j++], all_descs[j++]);
            get_src_tiles(grid_name).push_back(t);
        }
    }

    /* FIXME: check that the domains of remote procs don't overlap. */
    delete(all_descs);
}

void Router::read_netcdf(string filename, vector<int>& src_points,
                         vector<int>& dest_points, vector<double>& weights)
{
    /* open the remapping weights file */
    NcFile rmp_file(filename, NcFile::read);
    NcVar src_var = rmp_file.getVar("col");
    NcVar dest_var = rmp_file.getVar("row");
    NcVar weights_var = rmp_file.getVar("S");

    /* Assume that these are all 1 dimensional. */
    int src_size = src_var.getDim(0).getSize();
    int dest_size = src_var.getDim(0).getSize();
    int weights_size = src_var.getDim(0).getSize();
    assert(src_size == dest_size == weights_size);

    int *src_data = new int[src_size];
    int *dest_data = new int[dest_size];
    double *weights_data = new double[weights_size];

    src_var.getVar(src_data);
    dest_var.getVar(dest_data);
    weights_var.getVar(weights_data);

    src_points.insert(src_points.begin(), src_data, src_data + src_size);
    dest_points.insert(dest_points.begin(), dest_data, dest_data + dest_size);
    weights.insert(weights.begin(), weights_data, weights_data + weights_size);

    delete(src_data);
    delete(dest_data);
    delete(weights_data);
}


/* Will need to build different rules for the send_local case. */
void Router::build_routing_rules(void)
{
    /* Now open the grid remapping files created with ESMF. Use this to
     * populate the routing maps. */

    /* FIXME: performance may be a problem here. This loop is going to take in
     * the order of seconds for a 1000x1000 grid with 1000 procs, it may
     * not scale well to larger grids. Ways to improve performance:
     * 1. Set up a point -> proc map. This could be several Mb in size.
     * 2. Figure out the maximum number of times a local point can appear as a
     *    source point, exit loop after this number is reached.
     * 3. Tune to the particular layout of the remapping file, e.g. it looks
     *    like the dest points are presented in consecutive order. */

    /* Iterate over all the grids that we send to. */
    for (auto& grid : dest_grids) {
        vector<int> src_points;
        vector<int> dest_points;
        vector<double> weights;

        /* grid.first is the name, grid.second is the object. */
        read_netcdf(local_grid_name + "_to_" + grid.first + "_rmp.nc",
                    src_points, dest_points, weights);

        /* For all points that the local tile is responsible for, figure out
         * which remote tiles it needs to send to. FIXME: some kind of check
         * that all our local points are covered. */
        for (const auto point : local_tile->points) {
            for (int i = 0; i < src_points.size(); i++) {
                if ((src_points[i] == point) &&
                    (weights[i] > WEIGHT_THRESHOLD)) {
                    /* The src_point exists on the local tile. */

                    /* Search through the remote tiles and find the one that is
                     * responsible for the dest_point that corresponds to this
                     * src_point. */
                    for (auto *tile : grid.second->tiles) {
                        if (tile->has_point(dest_points[i])) {
                            /* So this src_point (on the local tile) needs to
                             * be sent to dest_point on the remote tile. We
                             * keep track of this by adding it to a list of
                             * send_points. */

                            /* All point references use a global point id,
                             * however when we actually go to send a field it
                             * is going to use an index which starts at 0.
                             * Therefore it's necessary to convert into the
                             * local coordinate system. */
                            point_t p = tile->global_to_local(src_points[i]);
                            tile->send_points.push_back(p);
                            tile->weights.push_back(weights[i]);
                            break;
                        }
                    }
                }
            }
        }

        /* Now clean up all the unused RemoteProcs that were inserted in
         * exchange_descriptions(). */
        remove_unreferenced_tiles(grid.second->tiles);
    }

    /* Iterate over all the grids that we receive from. */
    for (auto& grid : src_grids) {
        vector<int> src_points;
        vector<int> dest_points;
        vector<double> weights;

        read_netcdf(grid.first + "_to_" + local_grid_name + "_rmp.nc",
                    src_points, dest_points, weights);

        /* For all points that this tile is responsible for, figure out which
         * remote tiles it needs to receive from. */
        for (auto point : local_tile->points) {
            for (int i = 0; i < dest_points.size(); i++) {
                if ((dest_points[i] == point) &&
                    (weights[i] > WEIGHT_THRESHOLD)) {

                    /* Search through the remote tiles and find the one that is
                     * responsible for the src_point that corresponds to this
                     * dest_point. */
                    for (auto *tile : grid.second->tiles) {
                        if (tile->has_point(src_points[i])) {
                            /* Add to the list of points and weights. */
                            point_t p = tile->global_to_local(dest_points[i]);
                            tile->recv_points.push_back(p);
                            tile->weights.push_back(weights[i]);
                            break;
                        }
                    }
                }
            }
        }
        remove_unreferenced_tiles(grid.second->tiles);
    }

    /* Now we have a routing data structure that tells us which remote tiles
     * the local tile needs to communicate with. Also which local points need
     * to be sent/received to/from the remote tile. */

}

void Router::remove_unreferenced_tiles(list<Tile *> &to_clean)
{
    auto it = to_clean.begin();
    while (1) {
        if (!(*it)->has_send_points()) {
            it = to_clean.erase(it);
        } else {
            it++;
        }

        if (it == to_clean.end()) {
            break;
        }
    }
}

bool Router::is_dest_grid(string grid)
{
    for (const auto& kv : dest_grids) {
        if (kv.first == grid) {
            return true;
        }
    }

    return false;
}

bool Router::is_src_grid(string grid)
{
    for (const auto& kv : src_grids) {
        if (kv.first == grid) {
            return true;
        }
    }

    return false;
}

CouplingManager::CouplingManager(string config, string grid_name,
                                 int lis, int lie, int ljs, int lje,
                                 int gis, int gie, int gjs, int gje)
{
    list<string> dest_grids, src_grids;

    parse_config(config, dest_grids, src_grids);
    router = new Router(grid_name, dest_grids, src_grids,
                        lis, lie, ljs, lje, gis, gie, gjs, gje);
    router->exchange_descriptions();
    router->build_routing_rules();
}

/* Parse yaml config file. Find out which grids communicate and through which
 * fields. */
void CouplingManager::parse_config(string config, list<string>& dest_grids,
                                   list<string>& src_grids)
{
    YAML::Node grids, destinations, fields;

    grids = YAML::LoadFile(config)["grids"];

    /* Iterate over grids. */
    for (size_t i = 0; i < grids.size(); i++) {
        string src_grid = grids[i]["name"].as<string>();

        /* Iterate over destinations for this grid. */
        destinations = grids[i]["destinations"];
        for (size_t j = 0; j < destinations.size(); j++) {
            string dest_grid = destinations[j]["name"].as<string>();

            /* Not allowed to send to self (yet) */
            assert(dest_grid != src_grid);

            if (router->get_local_grid_name() == src_grid) {
                dest_grids.push_back(dest_grid);
            } else if (router->get_local_grid_name() == dest_grid) {
                src_grids.push_back(src_grid);
            }

            /* Iterate over fields for this destination. */
            for (size_t k = 0; k < fields.size(); k++) {
                string field_name = fields[k].as<string>();

                if (router->get_local_grid_name() == src_grid) {
                   dest_grid_to_fields[dest_grid].push_back(field_name);
                } else if (router->get_local_grid_name() == dest_grid) {
                   src_grid_to_fields[src_grid].push_back(field_name);
                }
            }
        }
    }
}

bool CouplingManager::can_send_field_to_grid(string field, string grid)
{
    for (const auto &f : dest_grid_to_fields[grid]) {
        if (f == field) {
            return true;
        }
    }
    return false;
}

bool CouplingManager::can_recv_field_from_grid(string field, string grid)
{
    for (const auto &f : src_grid_to_fields[grid]) {
        if (f == field) {
            return true;
        }
    }
    return false;
}


