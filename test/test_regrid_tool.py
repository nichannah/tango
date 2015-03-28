
from __future__ import print_function

import sys
import unittest
import os
import netCDF4 as nc
import tango as coupler
import ctypes as ct
import numpy as np
import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt

class TestRegrid(unittest.TestCase):
    """
    Test using tango as a stand-alone regridding tool.

    The upside of this is a single approach to do all regridding either
    stand-alone or within the model.
    Downsides: - it depends on MPI.
               - it cannot (easily) do 3d fields.

    These tests should be called from tango top-level dir with:
        mpirun -n 2 ./bin/python-mpi test/test_regrid_tool.py
    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommrank.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def test_regrid(self):
        """
        Regrid a single field from CORE to MOM grid.
        """

        config = os.path.join(self.test_dir,
                              'test_input-1_mappings-2_grids-192x94_to_1440x1080')
        if self.rank == 0:
            # Read in field to to regridded.
            with nc.Dataset(os.path.join(config, 'u_10.0001.nc')) as f:
                u = np.array(f.variables['U_10'][0,:,:], dtype='float64')

            print('send max(u) {}'.format(np.max(u)))

            tango = coupler.Tango(config, 'atm', 0, 192, 0, 94, 0, 192, 0, 94)
            tango.begin_transfer(0, 'ice')
            tango.put('u', u)
            tango.end_transfer()
        else:
            recv_u = np.zeros((1440, 1080), dtype='float64')
            tango = coupler.Tango(config, 'ice', 0, 1440, 0, 1080, 0, 1440, 0, 1080)
            tango.begin_transfer(0, 'atm')
            tango.get('u', recv_u)
            tango.end_transfer()

            print('recv max(u) {}'.format(np.max(recv_u)))
            plt.imshow(recv_u)
            plt.savefig('test.png')

            # Write out regridded field.
            #import pdb
            #pdb.set_trace()

        tango.finalize()


if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a chance for MPI_Finalize() to be called.
    try:
        unittest.main()
    except SystemExit:
        pass
