
#include "tango.h"
#include <iostream>

/* C++ stubs to test the Python interface. */

static string curr_field_name
static double *curr_array
static int size;

void tango_init(const char *grid_name,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje)
{}

void tango_begin_transfer(int time, const char* grid_name)
{}

void tango_put(const char* field_name, double array[], int size)
{

    curr_field_name = string(field_name);

    curr_array = new double[size];
    for (int i = 0; i < size; i++)
    {
        curr_array[i] = array[i];    
    }
}

void tango_get(const char* field_name, double array[], int size)
{
    assert(string(field_name) == curr_field_name);
    assert(size == cur_size);

    for (int i = 0; i < size; i++)
    {
        array[i] = curr_array[i];    
    }

    delete(curr_array);
}

void tango_end_transfer(void)
{}

void tango_finalize(void)
{}

