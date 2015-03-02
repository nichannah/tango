
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct
import numpy as np

send_sst = np.array([292.1, 295.7, 290.5, 287.9,
                     291.3, 294.3, 291.8, 290.0,
                     292.1, 295.2, 290.8, 284.7,
                     293.3, 290.1, 297.8, 293.4]);
send_sss = np.array([32.1, 33.7, 33.5, 34.9,
                     31.3, 35.3, 33.8, 36.0,
                     30.1, 31.2, 31.8, 37.7,
                     29.3, 34.1, 29.8, 39.4]);
# Air temp.
send_temp = np.array([302.1, 305.7, 300.5, 307.9,
                      301.3, 304.3, 301.8, 300.0,
                      302.1, 305.2, 300.8, 304.7,
                      303.3, 290.1, 397.8, 303.4]);

class TestMultipleTiles(unittest.TestCase):
    """
    Tests with only multiple tiles per grid.
    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommrank.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def test_one_way_send_receive(self):
        """
        Send a single array from one tile on the source grid to to two tiles on
        the destination grid.

        These tests should be called with:
            mpirun -n 3 ./bin/python-mpi test/test_multiple_tiles.py
        """

        grid_name = 'ocean'

        config = os.path.join(self.test_dir, 'test_input-1_mappings-2_grids')

        if self.rank == 0:
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 2, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_sst[0:8])
            tango.end_transfer()

        elif self.rank == 1:
            tango = coupler.Tango(config, grid_name, 0, 4, 2, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_sst[8:16])
            tango.end_transfer()

        else:
            recv_sst = np.ones(len(send_sst))

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_sst)
            tango.end_transfer()

            print('len(recv_sst): {}'.format(len(recv_sst)))
            print('recv_sst: {}'.format(recv_sst))
            #assert(np.array_equal(send_sst, recv_sst))

        tango.finalize()


if __name__ == '__main__':
    try:
        unittest.main()
    except SystemExit:
        pass
