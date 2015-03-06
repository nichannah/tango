#include <list>
#include <assert.h>
#include <mpi.h>

#include "tango.h"
#include "router.h"

using namespace std;

class Field {
public:
    double *buffer;
    unsigned int size;
    Field(double *buf, unsigned int buf_size);
};

Field::Field(double *buf, unsigned int buf_size)
    : buffer(buf), size(buf_size) {}

class PendingSend {
public:
    MPI_Request *request;
    double *buffer;
    PendingSend(MPI_Request *request, double *buffer);
};

PendingSend::PendingSend(MPI_Request *request, double *buffer)
    : request(request), buffer(buffer) {}

class Transfer {
private:
    int curr_time;
    /* Name of grid that this transfer is sending/recieving to/from. */
    string peer_grid;
public:
    unsigned int total_send_size;
    unsigned int total_recv_size;
    string get_peer_grid(void) const { return peer_grid; }
    list<Field> fields;
    list<PendingSend> pending_sends;
    Transfer(int time, string peer);
};

Transfer::Transfer(int time, string peer)
    : curr_time(time), peer_grid(peer), total_send_size(0), total_recv_size(0) {}

static Transfer *transfer;
static CouplingManager *cm;

/* FIXME: Check return codes of MPI calls. */

/* Pass in the grid name, the extents of the global domain and the extents of
 * the local domain that this proc is responsible for. */
void tango_init(const char *config, const char *grid_name,
               /* Local  domain */
               unsigned int lis, unsigned int lie,
               unsigned int ljs, unsigned int lje,
               /* Global domain */
               unsigned int gis, unsigned int gie,
               unsigned int gjs, unsigned int gje)
{
    transfer = nullptr;

    /* FIXME: what to do about Fortran indexing convention here. For the time
     * being stick to C++/Python. */

    /* Build the coupling manager for this process. This lasts for the lifetime
     * of the process. */
    cm = new CouplingManager(string(config), string(grid_name));
    cm->build_router(lis, lie, ljs, lje, gis, gie, gjs, gje);
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
    if (!cm->can_send_field_to_grid(string(field_name),
                                    transfer->get_peer_grid())) {
        cerr << "Error: according to config.yaml " << string(field_name)
             << " can't be put to " << transfer->get_peer_grid()
             << " grid" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Check that the field size is correct. */
    if (cm->expected_field_size() == size) {
        assert(false);
    }

    transfer->total_send_size += size;
    transfer->fields.push_back(Field(array, size));
}

void tango_get(const char *field_name, double array[], int size)
{
    assert(transfer != nullptr);
    assert(transfer->total_send_size == 0);
    if (!cm->can_recv_field_from_grid(string(field_name),
                                      transfer->get_peer_grid())) {
        cerr << "Error: according to config.yaml " << string(field_name)
             << " can't be get from " << transfer->get_peer_grid() << " grid "
             << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (cm->expected_field_size() == size) {
        assert(false);
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

    Router *router = cm->get_router();
    string peer_grid = transfer->get_peer_grid();

    /* We are the sender */
    if (transfer->total_send_size != 0) {

        /* Iterate over the mappings to other tiles that we have. */
        for (const auto *mapping : router->get_send_mappings(peer_grid)) {

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

            auto &remote_points = mapping->get_side_A_points()

            int count = remote_points->size() * transfer->fields.size();
            double *send_buf = new double[count];

            offset = 0;
            for (const auto& field : transfer->fields) {
                for (auto& rp : remote_points) {

                    send_buf[offset] = 0;

                    /* Get local points that correspond to this remote point
                     * apply the weights. */
                    for (auto& lp_and_weight : mapping->get_side_B(rp)) {
                        point_t local_point = lp_and_w.first;
                        double weight = lp_and_w.second;

                        send_buf[offset] += field.buffer[local_point] * weight;
                    }
                    offset++
                }
            }

            cout << "Sending a total of " << remote_points.size() << " points." << endl;
            cout << "Sum or array being sent: ";
            double sum = 0;
            for (unsigned int i = 0; i < remote_points.size(); i++) {
                sum += send_buf[i];
            }
            cout << sum << endl;

            /* Now do the actual send to the remote tile associated with this
             * mapping. */
            /* FIXME: tag? */
            MPI_Request *request = new MPI_Request;
            MPI_Isend(send_buf, count, MPI_DOUBLE, mapping->get_remote_tile_id(),
                      0x7A960, MPI_COMM_WORLD, request);

            /* Keep these, they need be freed later. */
            transfer->pending_sends.push_back(PendingSend(request, send_buf));
        }

    } else {
        /* We are the receiver. We do a blocking receive. */

        for (const auto *mapping : mapping->get_recv_mappings(peer_grid)) {

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
                     0x7A960, MPI_COMM_WORLD, &status);

            offset = 0;
            for (const auto& field : transfer->fields) {
                for (auto& lp : local_points) {
                    field.buffer[lp] += recv_buf[offset];
                    offset++;
                }
            }

            cout << "Receiving a total of " << local_points.size() << " points." << endl;
            cout << "Sum or array being received: ";
            double sum = 0;
            for (unsigned int i = 0; i < field.size(); i++) {
                sum += field.buffer[i];
            }
            cout << sum << endl;

            delete[] recv_buf;
        }
    }
}

void tango_finalize()
{
    complete_comms();
    assert(transfer == nullptr);

    delete cm;
    cm = nullptr;
}
