
#include <iostream>
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

int main(void)
{
    ptime start_time(date(1900, 1, 1));
    ptime end_time(date(1900, 1, 1) + days(365));

    time_iterator titr(start_time, hours(6));
    for (; titr <= end_time; ++titr) {
        cout << to_iso_extended_string(*titr) << endl;
    }

    return 0;
}
