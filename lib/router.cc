
#include <fstream>
#include <algorithm>
#include <netcdf>
#include <mpi.h>
#include <assert.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include "router.h"

using namespace netCDF;

#define MAX_GRID_NAME_SIZE 32
#define DESCRIPTION_SIZE (MAX_GRID_NAME_SIZE + 1 + 9)
#define WEIGHT_THRESHOLD 1e-12

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
    : id(tile_id), lis(lis), lie(lie), ljs(ljs), lje(lje),
      gis(gis), gie(gie), gjs(gjs), gje(gje)
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
    auto it = find(points.begin(), points.end(), p);
    return (it != points.end());
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

bool Tile::domain_equal(unsigned int lis, unsigned int lie, unsigned int ljs,
                        unsigned int lje, unsigned int gis, unsigned int gie,
                        unsigned int gjs, unsigned int gje)
{
    return ((this->lis == lis) && (this->lie == lie) && (this->ljs == ljs)
            && (this->lje == lje) && (this->gis == gis) && (this->gie == gie)
            && (this->gjs == gjs) && (this->gje == gje));
}

Router::Router(string grid_name,
               unordered_set<string>& dest_grid_names,
               unordered_set<string>& src_grid_names,
               unsigned int lis, unsigned int lie, unsigned int ljs,
               unsigned int lje, unsigned int gis, unsigned int gie,
               unsigned int gjs, unsigned int gje) : local_grid_name(grid_name)
{
    tile_id_t tile_id;

    MPI_Comm_rank(MPI_COMM_WORLD, &tile_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    local_tile = new Tile(tile_id, lis, lie, ljs, lje, gis, gie, gjs, gje);
    assert(dest_grid_names.find(local_grid_name) == dest_grid_names.end());
    assert(src_grid_names.find(local_grid_name) == src_grid_names.end());
    dest_grids = dest_grid_names;
    src_grids = src_grid_names;
}

Router::~Router()
{
    delete local_tile;

    for (auto& kv : grid_tiles) {
        for (auto *tile : kv.second) {
            delete tile;
        }
    }
}

/* Pack a description of this proc and broadcast it to all others. They'll use
 * the information to set up their routers. */
void Router::exchange_descriptions(void)
{
    int description[DESCRIPTION_SIZE];
    int *all_descs;
    unsigned int i;

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

    /* Distribute all_descriptions. There is a big design decision here. The
     * domain information of each PE/tile is distributed to all others, it is
     * then the responsibility of each PE to calculate the mappings that it is
     * involved in. This increases computation overall but saves a lot on
     * communication. */
    all_descs = new int[DESCRIPTION_SIZE * num_ranks];
    MPI_Gather(description, DESCRIPTION_SIZE, MPI_INT, all_descs,
               DESCRIPTION_SIZE, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(all_descs, DESCRIPTION_SIZE * num_ranks, MPI_INT,
              0, MPI_COMM_WORLD);

    /* Unmarshall into Tile objects. */
    for (int i = 0; i < num_ranks * DESCRIPTION_SIZE; i += DESCRIPTION_SIZE) {

        int j = i;
        string grid_name;

        for (; j < (i + MAX_GRID_NAME_SIZE); j++) {
            if (all_descs[j] != '\0') {
                grid_name.push_back((char)all_descs[j]);
            }
        }

        if (all_descs[j] == local_tile->id) {
            /* This is us. */
            continue;
        }

        /* Note that below no tiles are kept for grids that we don't
         * communicate with, including ourselves. */

        /* Make a new tile and insert into list of tiles for this grid. This
         * list of will be refined later, any tile that we don't actually
         * communicate with will be deleted. */
        if (is_peer_grid(grid_name)) {
            /* A tile could get big, so we make pointers and avoid copying. */
            Tile *t = new Tile(all_descs[j], all_descs[j+1], all_descs[j+2],
                               all_descs[j+3], all_descs[j+4], all_descs[j+5],
                               all_descs[j+6], all_descs[j+7], all_descs[j+8]);
            grid_tiles[grid_name].push_back(t);

            /* Now create the mappings from the local tile to this remote tile.
             * These will be populated later. Note that there can be both send
             * and receive mappings for a single tile. */
            if (is_send_grid(grid_name)) {
                Mapping *m = new Mapping(t);
                /* FIXME: check that the mapping being added is not a
                 * duplicate. */
                send_mapping.push_back(m);
            }

            if (is_recv_grid(grid_name)) {
                Mapping *m = new Mapping(t);
                /* FIXME: check that the mapping being added is not a
                 * duplicate. */
                recv_mapping.push_back(m);
            }
        }
    }

    /* FIXME: check that the domains of remote procs don't overlap. */
    delete[] all_descs;
}

void Router::read_netcdf(string filename, vector<unsigned int>& src_points,
                         vector<unsigned int>& dest_points,
                         vector<double>& weights)
{
    /* open the remapping weights file */
    NcFile rmp_file(filename, NcFile::read);
    NcVar src_var = rmp_file.getVar("col");
    NcVar dest_var = rmp_file.getVar("row");
    NcVar weights_var = rmp_file.getVar("S");

    /* Assume that these are all 1 dimensional. */
    unsigned int src_size = src_var.getDim(0).getSize();
    unsigned int dest_size = dest_var.getDim(0).getSize();
    unsigned int weights_size = weights_var.getDim(0).getSize();
    assert(src_size == dest_size);
    assert(dest_size == weights_size);

    unsigned int *src_data = new unsigned int[src_size];
    unsigned int *dest_data = new unsigned int[dest_size];
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


void Router::add_to_send_mapping(string grid, unsigned int src_point,
                                 unsigned int dest_point, double weight) 
{
    for (auto *tile : grid_tiles[grid]) {
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
            point_t p = local_tile->global_to_local_domain(src_points[i]);
            tile->send_points.push_back(p);
            tile->send_weights.push_back(weights[i]);
            break;
        }
    }
}

void Router::add_to_recv_mapping(string grid, unsigned int src_point,
                                 unsigned int dest_point, double weight) 
{
/* Search through the remote tiles and find the one that is
     * responsible for the src_point that corresponds to this
     * dest_point. */
    for (auto *tile : grid_tiles[grid]) {
        if (tile->has_point(src_points[i])) {
            /* Add to the list of points and weights. Once
             * again the recv_points/send_points is the index
             * that the local tile receives or sends on. */
            point_t p = local_tile->global_to_local_domain(dest_points[i]);
            tile->recv_points.push_back(p);
            tile->recv_weights.push_back(weights[i]);
            break;
        }
    }
}

void Router::build_routing_rules(string config_dir)
{
    /* Now open the grid remapping files created with ESMF. Use this to
     * populate the mapping graph. */

    /* FIXME: performance may be a problem here. This loop is may take in
     * the order of seconds for a 1000x1000 grid with 1000 procs, it may
     * not scale well to larger grids. Ways to improve performance:
     * 1. Set up a point -> proc map. This could be several Mb in size.
     * 2. Figure out the maximum number of times a local point can appear as a
     *    source point, exit loop after this number is reached.
     * 3. Tune to the particular layout of the remapping file, e.g. it looks
     *    like the dest points are presented in consecutive order. */

    /* Iterate over all the grids that we send to. */
    for (auto& grid : send_grids) {
        vector<unsigned int> src_points;
        vector<unsigned int> dest_points;
        vector<double> weights;

        string remap_file = config_dir + "/" + local_grid_name + "_to_" +
                            grid + "_rmp.nc";
        if (!file_exists(remap_file)) {
            cerr << "Error: " << remap_file << " does not exist." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        read_netcdf(remap_file, src_points, dest_points, weights);

        /* For all points that the local tile is responsible for set up a
         * mapping to a tile on the grid that we are sending to. */
        for (const auto point : local_tile->points) {
            for (unsigned int i = 0; i < src_points.size(); i++) {

                unsigned int src_point = src_points[i];
                unsigned int dest_point = dest_points[i];
                double weight = weights[i]

                if ((src_point == point) && (weight > WEIGHT_THRESHOLD)) {
                    /* So this source points exists on the local tile, also the
                     * weight is large enough to care about. */

                    /* Set up a mapping between this source point and the
                     * destination. */
                    add_to_send_mapping(grid, src_point, dest_point, weight);
               }
            }
        }
    }

    /* Iterate over all the grids that we receive from. */
    for (auto& grid : recv_grids) {
        vector<unsigned int> src_points;
        vector<unsigned int> dest_points;
        vector<double> weights;

        string remap_file = config_dir + "/" + grid + "_to_" +
                            local_grid_name + "_rmp.nc";
        if (!file_exists(remap_file)) {
            cerr << "Error: " << remap_file << " does not exist." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        read_netcdf(remap_file, src_points, dest_points, weights);

        /* For all points that this tile is responsible for, figure out which
         * remote tiles it needs to receive from. */
        for (auto point : local_tile->points) {
            for (unsigned int i = 0; i < dest_points.size(); i++) {
                if ((dest_points[i] == point) &&
                    (weights[i] > WEIGHT_THRESHOLD)) {

                    add_to_recv_mapping(grid, src_point, dest_point, weight);
                }
            }
        }
    }

    /* Now clean up all the unused Tiles that were inserted in
     * exchange_descriptions(). */
    remove_unreferenced_tiles();

    /* FIXME: Check that all our local points are covered. */

    /* Now we have a routing data structure that tells us which remote tiles
     * the local tile needs to communicate with. Also which local points need
     * to be sent/received to/from each remote tile. */
}

void Router::remove_unreferenced_tiles(void)
{
    for (auto& kv : grid_tiles) {
        auto it = kv.second.begin();
        while (1) {
            if (it == kv.second.end()) {
                break;
            }

            if ((*it)->send_points_empty() && (*it)->recv_points_empty()) {
                it = kv.second.erase(it);
            } else {
                it++;
            }
        }
    }
}

bool Router::is_peer_grid(string grid)
{
    if (is_send_grid(grid) or is_recv_grid(grid)) {
        return true;
    }
    return false;
}

bool Router::is_send_grid(string grid)
{
    if (send_grids.find(grid) != send_grids.end()) {
        return true;
    }
    return false;
}

bool Router::is_recv_grid(string grid)
{
    if (recv_grids.find(grid) != recv_grids.end()) {
        return true;
    }
    return false;
}


list<Tile *>& Router::get_grid_tiles(string grid)
{
    assert(is_peer_grid(grid));
    return grid_tiles[grid];
}

CouplingManager::CouplingManager(string config_dir, string grid_name)
    : config_dir(config_dir), grid_name(grid_name) {}

/* Separate this from the constructor to make testing easier. */
void CouplingManager::build_router(int lis, int lie, int ljs, int lje,
                                  int gis, int gie, int gjs, int gje)
{
    unordered_set<string> dest_grids, src_grids;

    parse_config(this->config_dir, this->grid_name, dest_grids, src_grids);

    assert(router == nullptr);
    router = new Router(this->grid_name, dest_grids, src_grids,
                        lis, lie, ljs, lje, gis, gie, gjs, gje);
    router->exchange_descriptions();
    router->build_routing_rules(this->config_dir);
}

/* Parse yaml config file. Find out which grids communicate and through which
 * fields. */
void CouplingManager::parse_config(string config_dir, string local_grid_name,
                                   unordered_set<string>& dest_grids,
                                   unordered_set<string>& src_grids)
{
    YAML::Node mappings, fields;

    string config_file = config_dir + "/config.yaml";
    if (!file_exists(config_file)) {
        cerr << "Error: " << config_file << " does not exist." << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    mappings = YAML::LoadFile(config_file)["mappings"];

    /* Iterate over mappings. */
    for (size_t i = 0; i < mappings.size(); i++) {
        string src_grid = mappings[i]["source_grid"].as<string>();
        string dest_grid = mappings[i]["destination_grid"].as<string>();

        /* Check that this combination has not already been seen. */
        if (dest_grids.find(dest_grid) != dest_grids.end() &&
            src_grids.find(src_grid) != src_grids.end()) {
            cerr << "Error: duplicate entry in grid." << endl;
            cerr << "Mapping with source_grid = " << src_grid << " and "
                 << " destination_grid = " << dest_grid
                 << " occurs more than once." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (local_grid_name == src_grid) {
            dest_grids.insert(dest_grid);
        } else if (local_grid_name == dest_grid) {
            src_grids.insert(src_grid);
        } else {
            continue;
        }

        fields = mappings[i]["fields"];
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

