
#include <assert.h>
#include <mpi.h>
#include <netcdf>

using namespace netCDF;

#define DESCRIPTION_SIZE 6

/* Parse config file for this grid. This will give us a global id of
 * for each grid (based on the name). */

/* We have to build up a routing table. For each variable we need two
 * lists: 1) a list of ranks to send to, 2) a list of ranks to receive
 * from. */

/* The routing table will be a map. The key will be the grid that this
 * grid is communicating with and whether it is a send or a receive,
 * the value will be a list of ranks. So actually we need a send map
 * and a receive map. */

/* Gather all discriptions together and then broadcast them. Allow each
 * proc to calculate their own routing table. */


Router::Router(string name, int gis, int gie, int gjs, int gje,
               int lis, int lie, int ljs, int lje)
    : grid_name(name)
      this.gis(gis), this.gie(gie), this.gjs(gjs), this.gje(gje)
      this.lis(lis), this.lie(lie), this.ljs(ljs), this.lje(lje)
{

    /* This is useful to know. */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    /* Convert the local domain into a 1-D array of global indices. e.g. on a
     * 2x2 grid the indices would be:
     * | 3 | 4 |
     * | 1 | 2 |
     * This is how the ESMF remapping files index points. */

    int n_cols = gje - gjs
    int n_local_rows = lie - lis; 
    int i_offset = lis - gis;
    int j_offset = ljs - gjs;

    for (int i = i_offset; i < n_local_rows; i++) {
        int index = (n_cols * i) + j_offset;

        for (int j = ljs; j < lje; j++) {
            grid_points.insert(index); 
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

                if (my_name == grid_name) {
                   send_grid_to_vars_map[dest_name].push_back(var_name);
                } else if (my_name == dest_name)
                   recv_grid_to_vars_map[grid_name].push_back(var_name);
                }
            }
        }
    }
}

/* Pack a description of this rank and broadcast it to all others. They'll use
 * the information to set up their routing tables. */
void Router::broadcast_descriptions(void)
{
    unsigned int description[DESCRIPTION_SIZE];
    unsigned int *all_descriptions;

    /* Construct description string for this proc. */
    description[0] = my_rank;
    description[1] = grid_id;
    description[2] = isd;
    description[3] = ied;
    description[4] = jsd;
    description[5] = jed;

    all_descriptions = new unsigned int[DESCRIPTION_SIZE * num_procs];

    /* Distribute all_descriptions. */
    MPI_Gather(description, DESCRIPTION_SIZE, MPI_UNSIGNED,
               all_descriptions, DESCRIPTION_SIZE * num_procs,
               MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Broadcast(all_descriptions, DESCRIPTION_SIZE * num_procs,
                  MPI_UNSIGNED, 0, MPI_COMM_WORLD);
}

void Router::build_rules(void)
{
    map<string, vector<string> >::iterator it
    ifstream remap_file;

    parse_config();
    broadcast_descriptions();

    /* Now open the grid remapping files created with ESMF. Use this to
     * populate the routing maps. */

    /* Iterate over all the grids that we send to. */
    for (it = send_grid_to_vars_map.begin();
         it != send_grid_to_vars_map.end(); ++it) {

        /* open the remapping weights file corrosponding to
         * <my_name>_to_<dest_name>_rmp.nc */
        NcFile rmp_file(my_name + "_to_" + it->first + "_rmp.nc", NcFile::read);

    }

    /* Next grids that we receive from. */
}

