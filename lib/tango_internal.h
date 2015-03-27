#pragma once

#include <list>
#include <mpi.h>

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

