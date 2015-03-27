
from __future__ import print_function

import tempfile
import numpy as np
import netCDF4 as nc
from square2scrip import LatLonGrid

class TestGrid:

    def test_make_square_grid(self):
        """
        Make a square grid and check against ESMF example grid.
        """

        (_, output) = tempfile.mkstemp()
        grid = LatLonGrid(144, 72)
        grid.write('', output)

        test_f = nc.Dataset(output)
        comp_f = nc.Dataset('ll2.5deg_grid.nc')

        for k in test_f.variables.keys():
            assert(np.array_equal(comp_f.variables[k][:],
                                  test_f.variables[k][:]))

        test_f.close()
        comp_f.close()
