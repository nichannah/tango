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

class Transaction {
private:
    int curr_time;
    int sender;
public:
    unsigned int total_send_size;
    unsigned int total_recv_size;
    queue<Field *> to_transfer;
    Transaction(int time);
};

Transaction::Transaction(int time)
    : curr_time(time), total_send_size(0), total_recv_size(0) {}

static Transaction *curr_transaction;
static Router *router;

/* Pass in the grid name, the extents of the global domain and the extents of
 * the local domain that this proc is responsible for. */
void tango_init(const char *grid_name,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje)
{
    curr_transaction = NULL;

    /* FIXME: what to do about Fortran indexing convention here. For the time
     * being stick to C++/Python. */

    /* Build the router. */
    router = new Router(grid_name, gis, gie, gjs, gje, lis, lie, ljs, lje);
    router.build_rules();
}

void tango_begin_transfer(int time, const char* with_grid)
{
    assert(curr_transaction == NULL);
    curr_transaction = new transaction(time);
}

void tango_put(const char *name, double array[], int size)
{
    field *f;
    assert(curr_transaction != NULL);
    assert(curr_transaction->total_recv_size == 0);

    curr_transaction->total_send_size += size;
    f = new field(array, size);
    curr_transaction->to_transfer.push(f);
}

void tango_get(const char *name, double array[], int size)
{
    field *f;
    assert(curr_transaction != NULL);
    assert(curr_transaction->total_send_size == 0);

    curr_transaction->total_recv_size += size;
    f = new field(array, size);
    curr_transaction->to_transfer.push(f);
}

void tango_end_transfer()
{
    double *send_buf, *recv_buf;
    int sender;
    field *f;
    unsigned int offset;

    assert(curr_transaction != NULL);
    assert(curr_transaction->total_send_size == 0 ||
           curr_transaction->total_recv_size == 0);
    assert(curr_transaction->total_send_size != 0 ||
           curr_transaction->total_recv_size != 0);

    send_buf = NULL;
    recv_buf = NULL;

    /* We are a sender */
    if (curr_transaction->total_send_size != 0) {
        sender = my_rank;

        /* Copy over all send fields to a single buffer. */
        send_buf = new double[curr_transaction->total_send_size];
        offset = 0;
        while (!curr_transaction->to_transfer.empty()) {
            f = curr_transaction->to_transfer.front();
            memcpy(&(send_buf[offset]), f->buffer, f->size);
            offset += f->size;

            curr_transaction->to_transfer.pop();
            delete(f);
        }
    } else {
        recv_buf = new double[curr_transaction->total_recv_size];
    }

    /* Do the transfer. */
    MPI_Scatter(send_buf, curr_transaction->total_send_size, MPI_DOUBLE,
                recv_buf, curr_transaction->total_recv_size, MPI_DOUBLE,
                sender, MPI_COMM_WORLD);

    /* Copy recv into individual field buffers. */
    if (curr_transaction->total_recv_size != 0) {
        offset = 0;
        while (!curr_transaction->to_transfer.empty()) {
            f = curr_transaction->to_transfer.front();
            memcpy(f->buffer, &(recv_buf[offset]), f->size);
            offset += f->size;

            curr_transaction->to_transfer.pop();
            delete(f);
        }
    }

    delete(send_buf);
    delete(recv_buf);
    delete(curr_transaction);
}

void tango_finalize()
{
    assert(curr_transaction == NULL);
    delete(router);
}
