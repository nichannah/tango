
#define MAX_SEND_RECV_BUFFERS 2

typedef struct buf_type {
    double *data;
    unsigned int size;
} buf_type;

static buf_type to_send[MAX_SEND_RECV_BUFFERS];
static buf_type to_recv[MAX_SEND_RECV_BUFFERS];
static unsigned int send_count, recv_count;
static unsigned int total_send_size, total_recv_size;
static bool in_progress;
static int curr_time;

bool tango_init()
{
    in_progress = false;
}

bool tango_begin_comm(int time)
{
    assert(!in_progress);
    in_progress = true;
    curr_time = time
    send_count = 0;
    recv_count = 0;
    total_send_size = 0;
    total_recv_size = 0;
}

bool tango_end_comm(int time)
{
    double *send_buf;
    int i;

    assert(in_progress); 

    /* Combine all buffers to send into one. */
    send_buf = (double *)malloc(total_send_size * sizeof(double));
    for (i = 0; i < send_count; i++) {

        for (j = 0; j < ) {
        }
    }


    in_progress = false
}

bool tango_send(double data[], int size)
{
    assert(in_progress);

    to_send[send_count].data = data;
    to_send[send_count].size = size;
    send_count++;

    total_send_size += size;
}

bool tango_recv(double data[], int size)
{
    assert(in_progress);

    to_recv[recv_count].data = data;
    to_recv[recv_count].size = size;
    recv_count++;

    total_recv_size += size;
}

bool tango_finalize()
{
    /* Free house-keeping. */
}

