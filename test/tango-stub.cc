
#include <iostream>
#include <assert.h>

#include "tango.h"

/* C++ stubs to test the Python interface. */


using namespace std;

static string curr_field_name;
static double *curr_array;
static int curr_size;

DLLEXPORT void tango_init(const char *config, const char *grid_name,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje)
{}

DLLEXPORT void tango_begin_transfer(int time, const char* grid_name)
{
    cout << "tango_begin_transfer() called" << endl;
    cout << "time = " << time << " grid_name = " << grid_name << endl;
}

DLLEXPORT void tango_put(const char* field_name, double array[], int size)
{

    curr_field_name = string(field_name);
    curr_size = size;

    curr_array = new double[size];
    for (int i = 0; i < size; i++)
    {
        curr_array[i] = array[i];
    }
}

DLLEXPORT void tango_get(const char* field_name, double array[], int size)
{
    assert(string(field_name) == curr_field_name);
    assert(size == curr_size);

    for (int i = 0; i < size; i++)
    {
        array[i] = curr_array[i];
    }

    delete(curr_array);
}

DLLEXPORT void tango_end_transfer(void)
{}

DLLEXPORT void tango_finalize(void)
{}

