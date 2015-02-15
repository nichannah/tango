#include <queue>

class field {
public:
    double *buffer
    unsigned int size;
    field(double *buf, unsigned int buf_size);
};

field::field(double *buf, unsigned int buf_size)
    : buffer(buf), size(buf_size) {}

class transaction {
private:
    int curr_time;
public:
    unsigned int total_send_size;
    unsigned int total_recv_size;
    queue<field *> to_send;
    queue<field *> to_recv;
    transaction(int time);
    add_send(double *array, unsigned int size);
    add_recv(double *array, unsigned int size);
};

transaction::transaction(int time)
    : curr_time(time), total_send_size(0), total_recv_size(0)  {}

void transaction::add_send(double *array, unsigned int size)
{
    total_send_size += size;
    f = new field(array, size);
    to_send.push(f);
}

void transaction::add_recv(double *array, unsigned int size)
{
    total_recv_size += size;
    f = new field(array, size);
    to_recv.push(f);
}

static transaction *curr_transaction;

void tango_init()
{
    curr_transaction = NULL;
}

void tango_transfer_transfer(int time)
{
    assert(curr_transaction == NULL);
    curr_transaction = new transaction;
}

void tango_put(double data[], int size)
{
    assert(curr_transaction != NULL);
    curr_transaction->add_send(data, size);
}

void tango_get(double data[], int size)
{
    assert(curr_transaction != NULL);
    curr_transaction->add_recv(data, size);
}

bool tango_end_transfer()
{
    double *send_buf, *recv_buf;
    field *f;
    unsigned int offset;
    assert(curr_transaction != NULL);

    /* First copy over all send arrays to a single buffer. */
    if (curr_transaction.total_send_size != 0) {
        send_buf = new double[curr_transaction.total_send_size];
        offset = 0;
        while (!curr_transaction.to_send.empty()) {
            f = curr_transaction.to_send.front();
            memcpy(&(send_buf[offset]), f->buffer, f->size);
            offset += f->size;

            curr_transaction.to_send.pop()
            delete(f);
        }
    }

        /* Do the transfer. */
        MPI_Scatter(send_buf, curr_transaction.total_send_size, 
                    );

        delete(send_buf);
 
    if (curr_transaction.total_recv_size != 0) {
        recv_buf = new double[curr_transaction.total_recv_size];

        /* Do the recv. */
        MPI_Gather();

        /* Copy recv into individual buffers. */
        offset = 0;
        while (!curr_transaction.to_recv.empty()) {
            f = curr_transaction.to_recv.front();
            memcpy(f->buffer, &(recv_buf[offset]), f->buffer, f->size);
            offset += f->size;

            curr_transaction.to_recv.pop()
            delete(f);
        }
        delete(recv_buf);
    }

    delete(curr_transaction);
}

bool tango_finalize()
{
    /* Free house-keeping. */
}

