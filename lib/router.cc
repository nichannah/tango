
#include <assert.h>
#include <mpi.h>
#include <netcdf>

#include "router.h"

using namespace netCDF;

#define DESCRIPTION_SIZE 6
#define WEIGHT_THRESHOLD 1e12

RemoteProc::RemoteProc(unsigned int grid_id, unsigned int rank,
                       int lis, int lie, int ljs, int lje)
    : this.rank(rank), this.grid_id(grid_id), 
      this.lis(lis), this.lie(lie), this.ljs(ljs), this.lje(lje)
{
}

Router::Router(string name, int lis, int lie, int ljs, int lje,
           int gis, int gie, int gjs, int gje)
    : grid_name(name), 
      this.lis(lis), this.lie(lie), this.ljs(ljs), this.lje(lje),
      this.gis(gis), this.gie(gie), this.gjs(gjs), this.gje(lje)
{
    /* This is useful to know. */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    my_grid = name; 

    /* Set up list of points that this proc is responsible for with a global
     * reference. */
    int n_cols = gje - gjs
    int n_local_rows = lie - lis; 
    int i_offset = lis - gis;
    int j_offset = ljs - gjs;

    for (int i = i_offset; i < n_local_rows; i++) {
        int index = (n_cols * i) + j_offset;

        for (int j = ljs; j < lje; j++) {
            points.insert(index); 
            index++;
        }
    }
}

/* Parse yaml config file. Find out which grids communicate and through which
 * variables. */
void Router::parse_config(void)
{
    ifstream config_file;
    YAML::Node grids, destinations, variables;

    config_file.open("config.yaml");
    grids = YAML::Load(config_file)["grids"];

    /* Iterate over grids. */
    for (size_t i = 0; i < grids.size(); i++) {
        string grid_name = grids[i]["name"];

        /* Iterate over destinations for this grid. */
        destinations = grids[i]["destinations"];
        for (size_t j = 0; j < destinations.size(); j++) {
            string dest_name = destinations[j]["name"];

            /* Iterate over variables for this destination. */
            variables = destinations[j]["vars"]
            for (size_t k = 0; k < variables.size(); k++) {
                var_name = variables[k];

                if (my_grid == grid_name) {
                   dest_grid_to_fields_map[dest_name].push_back(var_name);
                   dest_grids.insert(dest_name);
                   my_grid_id = i;
                } else if (my_grid == dest_name)
                   src_grid_to_fields_map[grid_name].push_back(var_name);
                   src_grids.insert(grid_name);
                }
            }
        }
    }
}

/* Pack a description of this proc and broadcast it to all others. They'll use
 * the information to set up their routers. */
void Router::build_descriptions(void)
{
    unsigned int description[DESCRIPTION_SIZE];
    unsigned int *all_descs;

    /* Marshall my description into an array. */
    description[0] = my_grid_id;
    description[1] = my_rank;
    description[2] = lis;
    description[3] = lie;
    description[4] = ljs;
    description[5] = lje;

    all_descs = new unsigned int[DESCRIPTION_SIZE * num_procs];

    /* Distribute all_descriptions. */
    MPI_Gather(description, DESCRIPTION_SIZE, MPI_UNSIGNED,
               all_descriptions, DESCRIPTION_SIZE * num_procs,
               MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Broadcast(all_descriptions, DESCRIPTION_SIZE * num_procs,
                  MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    /* Unmarshall into RemoteProc objects. */
    for (int i = 0; i < num_procs * DESCRIPTION_SIZE; i += DESCRIPTION_SIZE) {
        grid_id = all_descs[i]
       /* Only care about remote procs that are on a peer grid. */
        if (is_peer_grid(grid_id)) {
            rp = RemoteProc(grid_id, all_descs[i+1], all_descs[i+2],
                            all_descs[i+3], all_descs[i+4], all_descs[i+5]);
            /* Insert into maps, these will be refined later and unnecessary
             * procs deleted. */
            if (is_dest_grid(grid_id)) {
               dest_map[grid_id_to_name_map[grid_id]].push_back(rp);
            }
            if (is_src_grid(grid_id)) {
               src_map[grid_id_to_name_map[grid_id]].push_back(rp);
            }
        }
    }

    /* FIXME: check that the domains the remote procs don't overlap. */

    delete(all_descs);
}

void Router::build_rules(void)
{
    parse_config();
    broadcast_descriptions();

    /* Now open the grid remapping files created with ESMF. Use this to
     * populate the routing maps. */

    /* Iterate over all the grids that we send to. */
    for (auto& grid : dest_grids) {

        /* open the remapping weights file corrosponding to
         * <my_name>_to_<dest_name>_rmp.nc */
        NcFile rmp_file(my_name + "_to_" + grid + "_rmp.nc", NcFile::read);
        NcVar src_var = rmp_file.getVar("col");
        NcVar dest_var = rmp_file.getVar("row");
        NcVar weights_var = rmp_file.getVar("S");

        /* Assume that these are all 1 dimensional. */
        int src_size = src_var.getDim(0).getSize();
        int dest_size = src_var.getDim(0).getSize();
        int weights_size = src_var.getDim(0).getSize();
        assert(src_size == dest_size == weights_size);

        int *src_points = new int[src_size];
        int *dest_points = new int[dest_size];
        int *weights = new double[weights_size];

        src_var.getVar(src_points);
        dest_var.getVar(dest_points);
        weights_var.getVar(weights);
        
        /* For all points that this proc is responsible for, figure out which
         * proc it needs to be mapped to ...  */
        for (auto point : local_points) {
        
            for (int i = 0; i < src_points; i++) {
                if ((src_points[i] == point) &&
                    (weights[i] > WEIGHT_THRESHOLD)) {

                    /* Search through the remote procs and find the one that is
                     * responsible for the dest_points that corresponds to this
                     * src_point. */
                    for (auto const &rp : dest_map[grid]) {
                        if (rp.has_global_point(dest_points[i])) {
                            /* Add to the list of points and weights. */
                            rp.send_points.push_back(src_points[i]);
                            rp.send_weights.push_back(src_weights[i]);
                            break;
                        }
                    }
                }
            }
        }

        /* Now clean up all the unused RemoteProcs that were inserted in
         * build_descriptions(). */
        list<RemoteProc>::iterator it = dest_map[grid].begin();
        while (1) {
            if (*it.no_send_points()) {
                it = dest_map[grid].erase(it);
            }
            if (it == dest_map[grid].end()) {
                break;
            }

            it++;
        }

        delete(src_points);
        delete(dest_points);
        delete(weights);
    }

    /* Now do the above for receiving! */
}
