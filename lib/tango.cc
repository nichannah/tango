
#include <mpi.h>
#include <assert.h>

#include "tango.h"
#include "tango_internal.h"
#include "router.h"

#define TANGO_TAG 0x7A960

using namespace std;

static Transfer *transfer;
static Router *router;
static Config *config;

/* FIXME: Need to force user to use API according to the config file. */

/* FIXME: what to do about Fortran indexing convention here. For the time
 * being stick to C++/Python. */

/* Pass in the grid name, the extents of the global domain and the extents of
 * the local domain that this proc is responsible for. */
void tango_init(const char *config_dir, const char *grid_name,
               /* Local  domain */
               unsigned int lis, unsigned int lie,
               unsigned int ljs, unsigned int lje,
               /* Global domain */
               unsigned int gis, unsigned int gie,
               unsigned int gjs, unsigned int gje)
{
    transfer = nullptr;

    config = new Config(string(config_dir), string(grid_name));
    config->parse_config();

    router = new Router(*config, lis, lie, ljs, lje, gis, gie, gjs, gje);
}

static void complete_comms(void)
{
    if (transfer != nullptr) {
        /* A transfer object can be left over from a previous tango call. In
         * that case the MPI comms are not complete. */
        for (auto &ps : transfer->pending_sends) {
            MPI_Wait(ps.request, MPI_STATUS_IGNORE);
            delete ps.request;
            delete[] ps.buffer;
        }
        delete transfer;
        transfer = nullptr;
    }
}

void tango_begin_transfer(int time, const char* grid)
{
    complete_comms();
    transfer = new Transfer(time, string(grid));
}

void tango_put(const char *field_name, double array[], int size)
{
    assert(transfer != NULL);
    assert(transfer->total_recv_size == 0);
    if (!config->can_send_field_to_grid(string(field_name),
                                        transfer->get_peer_grid())) {
        cerr << "Error: according to config.yaml " << string(field_name)
             << " can't be put to " << transfer->get_peer_grid()
             << " grid" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Check that the field size is correct. */
    /*
    if (config->expected_field_size() == size) {
        assert(false);
    }
    */

    transfer->total_send_size += size;
    transfer->fields.push_back(Field(array, size));
}

void tango_get(const char *field_name, double array[], int size)
{
    assert(transfer != nullptr);
    assert(transfer->total_send_size == 0);
    if (!config->can_recv_field_from_grid(string(field_name),
                                          transfer->get_peer_grid())) {
        cerr << "Error: according to config.yaml " << string(field_name)
             << " can't be get from " << transfer->get_peer_grid() << " grid "
             << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /*
    if (cm->expected_field_size() == size) {
        assert(false);
    }
    */

    /* Zero the receive array. The get operation will add to the values in
     * this. */
    for (int i = 0; i < size; i++) {
        array[i] = 0;
    }

    transfer->total_recv_size += size;
    transfer->fields.push_back(Field(array, size));
}

void tango_end_transfer()
{
    unsigned int offset;

    assert(transfer != nullptr);
    /* Check that this is either all send or all receive. */
    assert(transfer->total_send_size == 0 || transfer->total_recv_size == 0);
    assert(transfer->total_send_size != 0 || transfer->total_recv_size != 0);

    string peer_grid = transfer->get_peer_grid();

    /* We are the sender */
    if (transfer->total_send_size != 0) {

        /* Iterate over the mappings to other tiles that we have. */
        for (const auto& mapping : router->get_send_mappings(peer_grid)) {

            /* Presently we only support applying interpolation weights on the
             * send side. At some point it may make sense to support receive
             * side weighting also. The benefit of doing this depends on things
             * such as the relative grid sizes and cost of moving data around.
             * Ideally the grid sizes are roughtly matched in which case it
             * makes no difference. */

            /* What we do here is:
             * 1) name the remote points as the 'side A' points. The 'A side'
             * can expect all weights to have already been applied. Local
             * points are side B.
             *
             * 2) Go through the remote points and for each one get the
             * associated local points and weights.
             *
             * 3) The local points are then used as indices into the buffer
             * being send, the weights are applied to these.
             *
             * 4) Send to remote tile associated with this mapping.
             */

            const auto &remote_points = mapping->get_side_A_points();

            int count = remote_points.size() * transfer->fields.size();
            double *send_buf = new double[count];

            offset = 0;
            for (const auto& field : transfer->fields) {
                for (auto& rp : remote_points) {

                    send_buf[offset] = 0;

                    /* Get local points that correspond to this remote point
                     * and apply weights. */
                    for (const auto& lp_and_w : mapping->get_side_B(rp)) {
                        point_t local_point = lp_and_w.first;
                        double weight = lp_and_w.second;

                        send_buf[offset] += field.buffer[local_point] * weight;
                    }
                    offset++;
                }
            }

            /* Now do the actual send to the remote tile associated with this
             * mapping. */
            MPI_Request *request = new MPI_Request;
            MPI_Isend(send_buf, count, MPI_DOUBLE, mapping->get_remote_tile_id(),
                      TANGO_TAG, MPI_COMM_WORLD, request);

            /* Keep these, they need be freed later. */
            transfer->pending_sends.push_back(PendingSend(request, send_buf));
        }

    } else {
        /* We are the receiver. We do a blocking receive. */

        for (const auto& mapping : router->get_recv_mappings(peer_grid)) {

            /* What we do here:
             *
             * 1) name the local points as the 'side A' points.
             *
             * 2) receive data from remote tile associated with this mapping.
             *
             * 3) the data comes in as a sequential array (of course) but each
             * array element can refer to any local point, so the data has to
             * be 'unboxed', i.e. loaded into the correct index of the receive
             * field.
             *
             * 4) Since many mappings can contribute to a single point we use
             * += to accumulate all incoming data for that points.
             */

            const auto& local_points = mapping->get_side_A_points();

            int count = local_points.size() * transfer->fields.size();
            double *recv_buf = new double[count];

            MPI_Status status;
            MPI_Recv(recv_buf, count, MPI_DOUBLE, mapping->get_remote_tile_id(),
                     TANGO_TAG, MPI_COMM_WORLD, &status);

            offset = 0;
            for (const auto& field : transfer->fields) {
                for (auto& lp : local_points) {
                    field.buffer[lp] += recv_buf[offset];
                    offset++;
                }
            }

            delete[] recv_buf;
        }
    }
}

void tango_finalize()
{
    complete_comms();
    assert(transfer == nullptr);

    delete router;
    delete config;
    router = nullptr;
}
