
#include <netcdf>
#include <mpi.h>
#include <assert.h>
#include <fstream>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include "router.h"

using namespace netCDF;

#define MAX_GRID_NAME_SIZE 32
#define DESCRIPTION_SIZE (MAX_GRID_NAME_SIZE + 1 + 9)
#define WEIGHT_THRESHOLD 1e12

static bool file_exists(string file)
{
    if (access(file.c_str(), F_OK) == -1) {
        return false;
    } else {
        return true;
    }
}

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

    for (int i = i_offset; i < (i_offset + n_local_rows); i++) {
        point_t index = (n_cols * i) + j_offset;

        for (int j = ljs; j < lje; j++) {
            /* Since the remapping scheme (ESMP) labels points starting at 1
             * (not 0) we need to do the same. */
            points.push_back(index + 1);
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

point_t Tile::global_to_local_domain(point_t global)
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
        assert(grid_name != local_grid_name);
        assert(dest_grids.find(grid_name) == dest_grids.end());
        dest_grids[grid_name] = new Grid(grid_name);
    }
    for (const auto& grid_name : src_grid_names) {
        assert(grid_name != local_grid_name);
        assert(src_grids.find(grid_name) == src_grids.end());
        src_grids[grid_name] = new Grid(grid_name);
    }
}

Router::~Router()
{
    delete local_tile;

    for (auto &g : dest_grids) {
        delete g.second;
    }

    for (auto &g : src_grids) {
        delete g.second;
    }
}


list<Tile *>& Router::get_dest_tiles(grid_t grid)
{
    auto it = dest_grids.find(grid);
    assert(it != dest_grids.end());
    return ((*it).second)->tiles;
}

list<Tile *>& Router::get_src_tiles(grid_t grid)
{
    auto it = src_grids.find(grid);
    assert(it != src_grids.end());
    return ((*it).second)->tiles;
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

    /* MPI_UNSIGNED has size 8 so we use MPI_INT. */
    /* Distribute all_descriptions. */
    all_descs = new unsigned int[DESCRIPTION_SIZE * num_ranks];
    MPI_Gather(description, DESCRIPTION_SIZE, MPI_UNSIGNED, all_descs,
               DESCRIPTION_SIZE, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(all_descs, DESCRIPTION_SIZE * num_ranks, MPI_UNSIGNED,
              0, MPI_COMM_WORLD);

    /* Unmarshall into Tile objects. */
    for (int i = 0; i < num_ranks * DESCRIPTION_SIZE; i += DESCRIPTION_SIZE) {

        int j = i;
        grid_t grid_name;

        for (; j < (i + MAX_GRID_NAME_SIZE); j++) {
            if (all_descs[j] != '\0') {
                grid_name.push_back((char)all_descs[j]);
            }
        }

        /* Note that below no tiles are made for grids that we don't
         * communicate with, including ourselves. */

        /* Insert into maps. These will be refined later and unnecessary
         * tiles that we don't actually communicate with will be deleted.
         * NOTE: a grid can be _both_ a destination and a source, in this  */
        if (is_send_grid(grid_name)) {
            /* A tile could get big, so we make pointers and avoid copying. */
            Tile *t = new Tile(all_descs[j], all_descs[j+1], all_descs[j+2],
                               all_descs[j+3], all_descs[j+4], all_descs[j+5],
                               all_descs[j+6], all_descs[j+7], all_descs[j+8]);
            get_dest_tiles(grid_name).push_back(t);
        }
        if (is_recv_grid(grid_name)) {
            Tile *t = new Tile(all_descs[j], all_descs[j+1], all_descs[j+2],
                               all_descs[j+3], all_descs[j+4], all_descs[j+5],
                               all_descs[j+6], all_descs[j+7], all_descs[j+8]);
            get_src_tiles(grid_name).push_back(t);
        }
    }

    /* FIXME: check that the domains of remote procs don't overlap. */
    delete[] all_descs;
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
    int dest_size = dest_var.getDim(0).getSize();
    int weights_size = weights_var.getDim(0).getSize();
    assert(src_size == dest_size);
    assert(dest_size == weights_size);

    int *src_data = new int[src_size];
    int *dest_data = new int[dest_size];
    double *weights_data = new double[weights_size];

    src_var.getVar(src_data);
    dest_var.getVar(dest_data);
    weights_var.getVar(weights_data);

    src_points.insert(src_points.begin(), src_data, src_data + src_size);
    dest_points.insert(dest_points.begin(), dest_data, dest_data + dest_size);
    weights.insert(weights.begin(), weights_data, weights_data + weights_size);

    delete[] src_data;
    delete[] dest_data;
    delete[] weights_data;
}


/* Will need to build different rules for the send_local case. */
void Router::build_routing_rules(string config_dir)
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
        string remap_file = config_dir + "/" + local_grid_name + "_to_" +
                            grid.first + "_rmp.nc";
        if (!file_exists(remap_file)) {
            cerr << "Error: " << remap_file << " does not exist." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        read_netcdf(remap_file, src_points, dest_points, weights);

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

        /* Now clean up all the unused Tiles that were inserted in
         * exchange_descriptions(). */
        remove_unreferenced_tiles(grid.second->tiles);
    }

    /* Iterate over all the grids that we receive from. */
    for (auto& grid : src_grids) {
        vector<int> src_points;
        vector<int> dest_points;
        vector<double> weights;

        string remap_file = config_dir + "/" + grid.first + "_to_" +
                            local_grid_name + "_rmp.nc";
        if (!file_exists(remap_file)) {
            cerr << "Error: " << remap_file << " does not exist." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        read_netcdf(remap_file, src_points, dest_points, weights);

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

void Router::remove_unreferenced_tiles(list<Tile *> &to_clean, string type)
{
    assert(type == "send" || type == "recv");

    auto it = to_clean.begin();
    while (1) {
        if (type == "send") {
            if ((*it)->send_points_empty()) {
                it = to_clean.erase(it);
            } else {
                it++;
            }
        } else {
            if ((*it)->recv_points_empty()) {
                it = to_clean.erase(it);
            } else {
                it++;
            }
        }

        if (it == to_clean.end()) {
            break;
        }
    }
}

bool Router::is_send_grid(grid_t grid)
{
    for (const auto& kv : put_mappings) {
        if (kv.first == grid) {
            return true;
        }
    }

    return false;
}

bool Router::is_recv_grid(grid_t grid)
{
    for (const auto& kv : get_mappings) {
        if (kv.first == grid) {
            return true;
        }
    }

    return false;
}

CouplingManager::CouplingManager(string config_dir, string grid_name)
    : config_dir(config_dir), grid_name(grid_name) {}

/* Separate this from the constructor to make testing easier. */
void CouplingManager::build_router(int lis, int lie, int ljs, int lje,
                                   int gis, int gie, int gjs, int gje)
{
    list<grid_t> dest_grids, src_grids;

    parse_config(this->config_dir, this->grid_name, dest_grids, src_grids);

    router = new Router(this->grid_name, dest_grids, src_grids,
                        lis, lie, ljs, lje, gis, gie, gjs, gje);
    router->exchange_descriptions();
    router->build_routing_rules(this->config_dir);
}

/* Parse yaml config file. Find out which grids communicate and through which
 * fields. */
void CouplingManager::parse_config(string config_dir, grid_t local_grid_name,
                                   list<grid_t>& dest_grids,
                                   list<grid_t>& src_grids)
{
    YAML::Node grids, destinations, fields;

    string config_file = config_dir + "/config.yaml";
    if (!file_exists(config_file)) {
        cerr << "Error: " << config_file << " does not exist." << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    grids = YAML::LoadFile(config_file)["grids"];

    /* Iterate over grids. */
    for (size_t i = 0; i < grids.size(); i++) {
        string src_grid = grids[i]["name"].as<string>();

        /* Iterate over destinations for this grid. */
        destinations = grids[i]["destinations"];
        for (size_t j = 0; j < destinations.size(); j++) {
            string dest_grid = destinations[j]["name"].as<string>();

            /* Not allowed to send to self (yet) */
            assert(dest_grid != src_grid);

            if (local_grid_name == src_grid) {
                dest_grids.push_back(dest_grid);
            } else if (local_grid_name == dest_grid) {
                src_grids.push_back(src_grid);
            }

            /* Iterate over fields for this destination. */
            fields = destinations[j]["fields"];
            for (size_t k = 0; k < fields.size(); k++) {
                string field_name = fields[k].as<string>();

                if (local_grid_name == src_grid) {
                   dest_grid_to_fields[dest_grid].push_back(field_name);
                } else if (local_grid_name == dest_grid) {
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

