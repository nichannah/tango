
#include <mpi.h>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <yaml-cpp/yaml.h>

#include "tango.h"

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

int main(int argc, char* argv[])
{
    YAML::Node coupling_fields, mapping;
    int g_rows = 1080, g_cols = 1440, l_rows = 1080, l_cols = 1440;
    double *recv = new double[l_rows * l_cols];

    mapping = YAML::LoadFile("config.yaml")["mappings"][0];
    coupling_fields = mapping["fields"];

    MPI_Init(&argc, &argv);
    tango_init("./", "ice", 0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols);

    ptime start_time(date(1900, 1, 1));
    ptime end_time(date(1900, 1, 1) + days(365));
    time_iterator titr(start_time, hours(6));

    for (; titr <= end_time; ++titr) {
        string timestamp = to_iso_extended_string(*titr);

        tango_begin_transfer(timestamp.c_str(), "atm");

        for (size_t k = 0; k < coupling_fields.size(); k++) {
            string field_name = coupling_fields[k].as<string>();
            tango_get(field_name.c_str(), recv, l_rows * l_cols);
        }

        tango_end_transfer();
    }
    tango_finalize();
    MPI_Finalize();

    delete recv;
}
