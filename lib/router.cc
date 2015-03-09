
#include <algorithm>
#include <mpi.h>
#include <assert.h>
#include <unistd.h>

#include "router.h"

#define MAX_GRID_NAME_SIZE 32
#define DESCRIPTION_SIZE (MAX_GRID_NAME_SIZE + 9)
#define WEIGHT_THRESHOLD 1e-12


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

point_t Tile::global_to_local_domain(point_t global) const
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

/* FIXME: override == operator for tiles. */
bool Tile::domain_equal(const shared_ptr<Tile>& another_tile) const
{
    return ((lis == another_tile->lis) && (lie == another_tile->lie) &&
            (ljs == another_tile->ljs) && (lje == another_tile->lje) &&
            (gis == another_tile->gis) && (gie == another_tile->gie) &&
            (gjs == another_tile->gjs) && (gje == another_tile->gje));
}

void Tile::pack(int *box, size_t size)
{
    assert(size == 9);

    box[0] = id;
    box[1] = lis;
    box[2] = lie;
    box[3] = ljs;
    box[4] = lje;
    box[5] = gis;
    box[6] = gie;
    box[7] = gjs;
    box[8] = gje;
}

Router::Router(const Config& config,
               unsigned int lis, unsigned int lie, unsigned int ljs,
               unsigned int lje, unsigned int gis, unsigned int gie,
               unsigned int gjs, unsigned int gje)
    : config(config)
{
    
    tile_id_t tile_id;

    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &tile_id);

    unique_ptr<Tile> tmp(new Tile(tile_id, lis, lie, ljs, lje, gis, gie, gjs, gje));
    local_tile = move(tmp);

    exchange_descriptions();
    build_routing_rules();
}

Router::~Router()
{
    /*
    for (auto& kv : send_mappings) {
        while (!kv.second.empty()) {
            delete kv.second.front();
            kv.second.pop_front();
        }
    }

    for (auto& kv : recv_mappings) {
        while (!kv.second.empty()) {
            delete kv.second.front();
            kv.second.pop_front();
        }
    }
    */
}

void Router::create_send_mapping(string grid_name, shared_ptr<Tile> t)
{
    /* Check that the mapping being added is not a duplicate. */
    for (const auto *m : send_mappings[grid_name]) {
        assert(!t->domain_equal(m->get_remote_tile()));
    }

    Mapping *new_m = new Mapping(t);
    send_mappings[grid_name].push_back(new_m);
}

void Router::create_recv_mapping(string grid_name, shared_ptr<Tile> t)
{
    for (const auto *m : recv_mappings[grid_name]) {
        assert(!t->domain_equal(m->get_remote_tile()));
    }

    Mapping *new_m = new Mapping(t);
    recv_mappings[grid_name].push_back(new_m);
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
    string local_grid_name = config.get_local_grid();
    assert(local_grid_name.size() <= MAX_GRID_NAME_SIZE);
    for (i = 0; i < MAX_GRID_NAME_SIZE; i++) {
        if (i < local_grid_name.size()) {
            description[i] = local_grid_name[i];
            assert(description[i] != '\0');
        } else {
            description[i] = '\0';
        }
    }

    local_tile->pack(&(description[i]), DESCRIPTION_SIZE - MAX_GRID_NAME_SIZE);

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

        if (all_descs[j] == local_tile->get_id()) {
            /* This is us. */
            continue;
        }

        /* Note that below no tiles are kept for grids that we don't
         * communicate with, including ourselves. */

        /* Make a new tile and mappings to grid_name. Any tile that we don't
         * actually communicate with will be deleted. */
        if (config.is_peer_grid(grid_name)) {
            /* A tile could get big, so we make pointers and avoid copying. */
            shared_ptr<Tile> t(new Tile(all_descs[j], all_descs[j+1], all_descs[j+2],
                               all_descs[j+3], all_descs[j+4], all_descs[j+5],
                               all_descs[j+6], all_descs[j+7], all_descs[j+8]));

            /* Now create the mappings from the local tile to this remote tile.
             * These will be populated later. Note that there can be both send
             * and receive mappings for a single tile. */
            if (config.is_send_grid(grid_name)) {
                create_send_mapping(grid_name, t);
            }

            if (config.is_recv_grid(grid_name)) {
                create_recv_mapping(grid_name, t);
            }
        }
    }

    /* FIXME: check that the domains of remote procs don't overlap. */
    delete[] all_descs;
}


void Router::add_link_to_send_mapping(string grid, unsigned int src_point,
                                      unsigned int dest_point, double weight)
{
    for (auto *mapping : get_send_mappings(grid)) {

        /* The tile that this mapping leads to. */
        const shared_ptr<Tile>& remote_tile = mapping->get_remote_tile();
        if (remote_tile->has_point(dest_point)) {

            /* So we have found a remote tile which is responsible for our
             * destination point. We need populate the mapping object with
             * this src_point -> dest_point relationship. */

            /* We name the remote point as a 'side A' point. This is a
             * convention used to keep the Mapping agnostic re the send and
             * receive sides. 'side A' is the one that has it's points sent
             * between tiles. Presently in a tango put/send side A is the
             * remote side and in a get/receive it is the local side. In the
             * future we may allow for the sender to have it's point sent over
             * the wire. In that case side A would be the local side in a
             * put/send. */

            /* One more thing: all points are on the global domain by default
             * (that's how they're stored in the mapping weights file of
             * course), but to be useful on the local PE they need to be
             * converted to the local coordinate system. Essentially to an
             * array index in the local domain. */

            point_t side_A = remote_tile->global_to_local_domain(dest_point);
            point_t side_B = local_tile->global_to_local_domain(src_point);
            mapping->add_link(side_A, side_B, weight);

            /* Break now because there can be only one remote tile that has
             * this destination point. */
            break;
        }
    }
}

void Router::add_link_to_recv_mapping(string grid, unsigned int src_point,
                                      unsigned int dest_point, double weight) 
{
    /* See comments above for explanation of this function. */
    for (auto *mapping : get_recv_mappings(grid)) {

        const shared_ptr<Tile>& remote_tile = mapping->get_remote_tile();
        if (remote_tile->has_point(src_point)) {

            point_t side_A = local_tile->global_to_local_domain(dest_point);
            point_t side_B = remote_tile->global_to_local_domain(src_point);
            mapping->add_link(side_A, side_B, weight);
            break;
        }
    }
}

void Router::build_routing_rules(void)
{
    /* Now open the grid remapping files created with ESMF. Use this to
     * populate the mapping graph. */

    /* FIXME: performance may be a problem here. This loop may take in
     * the order of seconds for a 1000x1000 grid with 1000 procs, it may
     * not scale well to larger grids. Ways to improve performance:
     * 1. Set up a point -> tile/PE map. This could be several Mb in size.
     * 2. Figure out the maximum number of times a local point can appear as a
     *    source point, exit loop after this number is reached.
     * 3. Tune to the particular layout of the remapping file, e.g. it looks
     *    like the dest points are presented in consecutive order.
     * 4. Combination of above: and sort points before iterating over them. */

    /* Iterate over all the grids that we send to. */
    for (const auto& grid : config.get_send_grids()) {
        vector<unsigned int> src_points;
        vector<unsigned int> dest_points;
        vector<double> weights;

        config.read_weights(config.get_local_grid(), grid,
                            src_points, dest_points, weights);

        /* For all points that the local tile is responsible for set up a
         * mapping to a tile on the grid that we are sending to. */
        for (const auto point : local_tile->get_points()) {
            for (unsigned int i = 0; i < src_points.size(); i++) {

                unsigned int src_point = src_points[i];
                unsigned int dest_point = dest_points[i];
                double weight = weights[i];

                if ((src_point == point) && (weight > WEIGHT_THRESHOLD)) {
                    /* So this source points exists on the local tile, also the
                     * weight is large enough to care about. */

                    /* Set up a mapping between this source point and the
                     * destination. */
                    add_link_to_send_mapping(grid, src_point, dest_point, weight);
               }
            }
        }
    }

    /* Iterate over all the grids that we receive from. */
    for (const auto& grid : config.get_recv_grids()) {
        vector<unsigned int> src_points;
        vector<unsigned int> dest_points;
        vector<double> weights;

        config.read_weights(grid, config.get_local_grid(),
                            src_points, dest_points, weights);

        /* For all points that this tile is responsible for, figure out which
         * remote tiles it needs to receive from. */
        for (const auto point : local_tile->get_points()) {
            for (unsigned int i = 0; i < dest_points.size(); i++) {

                unsigned int src_point = src_points[i];
                unsigned int dest_point = dest_points[i];
                double weight = weights[i];

                if ((dest_point == point) && (weight > WEIGHT_THRESHOLD)) {
                    add_link_to_recv_mapping(grid, src_point, dest_point, weight);
                }
            }
        }
    }

    /* Now clean up all the unused mappings that were inserted in
     * exchange_descriptions(). Further description at function. */
    remove_unused_mappings();

    /* FIXME: Check that all our local points are covered get mapped to
     * somewhere. */

    /* Now we have a routing data structure that tells us which remote tiles
     * the local tile needs to communicate with. Also which local points need
     * to be sent/received to/from each remote tile. */
}

/* To begin with the router created mappings (and tiles) to represent all tiles
 * on the grids that this grid commuicates with. These were necessary because
 * we needed to know the domains/points of peer grids. However after actually
 * calculating the mappings many of these are empty, i.e. there are no points
 * on the local tile that map to a particaular remote tile. So remove all these
 * unused mappings. */
void Router::remove_unused_mappings(void)
{
    auto clean_func = [](list<Mapping *>& mappings) {
        auto it = mappings.begin();
        while (it != mappings.end()) {

            if ((*it)->not_in_use()) {
                it = mappings.erase(it);
            }
            else {
                it++;
            }
        }
        assert(!mappings.empty());
    };

    for (auto kv : send_mappings) {
        clean_func(kv.second);
    }

    for (auto kv : recv_mappings) {
        clean_func(kv.second);
    }
}
