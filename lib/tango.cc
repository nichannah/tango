#include <queue>
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

class Transfer {
private:
    int curr_time;
    int sender;
    /* The grid that this transfer is sending/recieving to/from. */
    string grid;
public:
    unsigned int total_send_size;
    unsigned int total_recv_size;
    queue<Field> fields;
    Transfer(int time, string peer);
};

Transfer::Transfer(int time, string peer)
    : curr_time(time), peer_grid(peer), total_send_size(0), total_recv_size(0) {}

/* FIXME: this should be a map, indexed by grid name. This will allow multiple
 * tango_init's per executable, necessary for send_self. */
static Transfer *transfer;
static Router *router;

/* Pass in the grid name, the extents of the global domain and the extents of
 * the local domain that this proc is responsible for. */
void tango_init(const char *grid_name,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje)
{
    transfer = NULL;

    /* FIXME: what to do about Fortran indexing convention here. For the time
     * being stick to C++/Python. */

    /* Build the router. */
    router = new Router(string(grid_name), lis, lie, ljs, lje,
                        gis, gie, gjs, gje);
    router.build_routing_rules();
}

void tango_begin_transfer(int time, const char* grid)
{
    assert(transfer == NULL);
    transfer = new Transfer(time, string(grid));
}

void tango_put(const char *var_name, double array[], int size)
{
    field *f;
    assert(transfer != NULL);
    assert(transfer->total_recv_size == 0);

    /* FIXME: check that this variable can be sent to the peer grid. */

    transfer->total_send_size += size;
    transfer->fields.push(Field(array, size));
}

void tango_get(const char *name, double array[], int size)
{
    field *f;
    assert(transfer != nullptr);
    assert(transfer->total_send_size == 0);

    /* FIXME: check that this variable can be received from the peer grid. */

    transfer->total_recv_size += size;
    transfer->fields.push(Field(array, size));
}

void tango_end_transfer()
{
    int sender;
    unsigned int offset;

    assert(transfer != nullptr);
    /* Check that this is either all send or all receive. */
    assert(transfer->total_send_size == 0 ||
           transfer->total_recv_size == 0);
    assert(transfer->total_send_size != 0 ||
           transfer->total_recv_size != 0);

    /* We are the sender */
    if (transfer->total_send_size != 0) {

        /* Iterate over the procs we are sending to. */
        for (auto& proc : router->get_dest_procs(transfer->grid)) {

            auto& points = proc.get_points();
            auto& weights = prog.get_weights();

            /* Marshall data into buffer for current send. All variables are
             * sent at once. */
            double send_buf[transfer->total_send_size];
            offset = 0;
            for (auto& field : transfer->fields) {
                for (int i = 0; i < points.size(); i++) {

                    send_buf[offset] = field.buffer[points[i]] * weight;
                    offset++;
                }
            }

            /* Now do the actual send. */
            MPI_Isend();
        }

    } else {

        /* We are the receiver. */
        /* Receive from source ranks. */
        /* Copy recv into individual field buffers. */
    }

    /* Transfer is complete. */
    delete(transfer);
}

void tango_finalize()
{
    assert(transfer == nullptr);
    delete(router);
    delete(local_proc);
}
