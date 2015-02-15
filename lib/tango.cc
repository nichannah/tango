#include <queue>
#include <assert.h>
#include <mpi.h>
#include "tango.h"

#define MAX_GRID_NAME_LEN 32

class field {
public:
    double *buffer;
    unsigned int size;
    field(double *buf, unsigned int buf_size);
};

field::field(double *buf, unsigned int buf_size)
    : buffer(buf), size(buf_size) {}

class transaction {
private:
    int curr_time;
    int sender;
public:
    unsigned int total_send_size;
    unsigned int total_recv_size;
    std::queue<field *> to_transfer;
    transaction(int time);
};

transaction::transaction(int time)
    : curr_time(time), total_send_size(0), total_recv_size(0) {}

typedef struct description {
    unsigned int rank;
    unsigned int isd, ied, jsd, jed;
    char grid_name[MAX_GRID_NAME_LEN];
} desc_type;

static transaction *curr_transaction;
static char* grid_name;

static void pack_description();
static void pack_description(int rank, isd, ied, jsd, jed, const char *name,
                             desc_type *desc)
{
    unsigned int i;

    desc->rank = rank;
    desc->isd = isd;
    desc->ied = ied;
    desc->jsd = jsd;
    desc->jed = jed;

    assert(strlen(name) < MAX_GRID_NAME_LEN);
    bzero(desc->grid_name, MAX_GRID_NAME_LEN);
    strcpy(desc->grid_name, name);
}

/* Pass in the grid name and the extents of the compute domain that this proc
 * is responsible for. */ 
void tango_init(const char *name, unsigned int isd, unsigned int ied,
                unsigned int jsd, unsigned int jed)
{
    int my_rank, size;
    desc_type my_description;
    MPI_Datatype desc_type_mpi;

    curr_transaction = NULL;
    grid_name = name;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Construct description string for this proc. */
    pack_description(my_rank, isd, ied, jsd, jed, grid_name, &my_description);
    MPI_Type_contiguous(sizeof(desc_type), MPI_CHAR, &desc_type_mpi); 
    MPI_Type_commit(&desc_type_mpi);

    if (my_rank == 0) {
        /* Parse config file for this grid. */

        /* Read in remapping weights file. */

        /* Get proc descriptions. */
        all_descriptions = new desc_type[size];
        MPI_Gather(my_description, 1, desc_type_mpi,
                   all_descriptions, size, desc_type_mpi,
                   0, MPI_COMM_WORLD);

        /* Tell each proc who they should send to. */

        /* Tell each proc who they should recv from. */
        
    } else {
        /* Each rank will be associated with a grid, send my description to the
         * root process. */
        MPI_Gather(my_description, MAX_DESCRIPTION_LEN, MPI_CHAR,
                   descriptions, 0, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
}

void tango_begin_transfer(int time)
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
        /* Get rank of sender. */
        MPI_Comm_rank(MPI_COMM_WORLD, &sender);

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
}
