
#include "config.h"

static bool file_exists(string file)
{
    if (access(file.c_str(), F_OK) == -1) {
        return false;
    } else {
        return true;
    }
}

/* Parse yaml config file. Find out which grids communicate and through which
 * fields. */
void Config::parse_config(void)
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
        string recv_grid = mappings[i]["source_grid"].as<string>();
        string send_grid = mappings[i]["destination_grid"].as<string>();

        /* Check that this combination has not already been seen. */
        if (send_grids.find(send_grid) != send_grids.end() &&
            recv_grids.find(recv_grid) != recv_grids.end()) {
            cerr << "Error: duplicate entry in grid." << endl;
            cerr << "Mapping with source_grid = " << recv_grid << " and "
                 << " destination_grid = " << send_grid
                 << " occurs more than once." << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (local_grid_name == recv_grid) {
            send_grids.insert(send_grid);
        } else if (local_grid_name == send_grid) {
            recv_grids.insert(recv_grid);
        } else {
            continue;
        }

        fields = mappings[i]["fields"];
        for (size_t k = 0; k < fields.size(); k++) {
            string field_name = fields[k].as<string>();

            if (local_grid_name == recv_grid) {
               send_grid_to_fields_map[send_grid].push_back(field_name);
            } else if (local_grid_name == send_grid) {
               recv_grid_to_fields_map[recv_grid].push_back(field_name);
            }
        }
    }
}

bool Config::can_send_field_to_grid(string field, string grid)
{
    for (const auto &f : send_grid_to_fields_map[grid]) {
        if (f == field) {
            return true;
        }
    }
    return false;
}

bool Config::can_recv_field_from_grid(string field, string grid)
{
    for (const auto &f : recv_grid_to_fields_map[grid]) {
        if (f == field) {
            return true;
        }
    }
    return false;
}

/* Return the permutation that sorts a vector. */
vector<unsigned int> sort_permutation(vector<unsigned int> const& vec)
{
    vector<unsigned int> perm(vec.size());
    iota(perm.begin(), perm.end(), 0);
    sort(perm.begin(), perm.end(),
        [&](unsigned int i, unsigned int j){ return vec[i] < vec[j]; });

    return perm;
}

/* Apply a permutation to vector. */
/* template <typename T> */
void apply_permutation(vector<unsigned int>& vec,
                       vector<unsigned int> const& perm)
{
    transform(perm.begin(), perm.end(), vec.begin(),
        [&](unsigned int i){ return vec[i]; });
}


/* Read the weights from the remapping weights file.
 * This method will also sort either the src or dest points in ascending order.
 * The sort is tricky because the relative positions of elements in the src,
 * dest and weights matter. i.e. src[0] <-> dest[0] <-> weights[0]. So
 * any changes made to src order also need to be made to dest and
 * weights order. */

void Config::read_weights(string src_grid, string dest_grid,
                          vector<unsigned int>& src_points,
                          vector<unsigned int>& dest_points,
                          vector<double>& weights, bool sort_src) const
{
    string remap_file = config_dir + "/" + src_grid + "_to_" +
                        dest_grid + "_rmp.nc";
    if (!file_exists(remap_file)) {
        cerr << "Error: " << remap_file << " does not exist." << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* open the remapping weights file */
    NcFile rmp_file(remap_file, NcFile::read);
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

    vector<unsigned int> perm;
    if (sort_src) {
        perm = sort_permutation(src_points);
    } else {
        perm = sort_permutation(dest_points);
    }
    apply_permutation(src_points, perm);
    apply_permutation(dest_points, perm);
    /*
    apply_permutation(weights, perm);
    */

    delete[] src_data;
    delete[] dest_data;
    delete[] weights_data;
}

bool Config::is_peer_grid(string grid) const
{
    if (is_send_grid(grid) or is_recv_grid(grid)) {
        return true;
    }
    return false;
}

bool Config::is_send_grid(string grid) const
{
    if (send_grids.find(grid) != send_grids.end()) {
        return true;
    }
    return false;
}

bool Config::is_recv_grid(string grid) const
{
    if (recv_grids.find(grid) != recv_grids.end()) {
        return true;
    }
    return false;
}

