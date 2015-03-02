
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct
import numpy as np

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

        send_sst = np.arange(16.0)
        config = os.path.join(self.test_dir, 'test_input-1_mappings-2_grids')

        if self.rank == 0:
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 2, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            # Copy into a consective array. */
            tmp = np.array((send_sst.reshape(4, 4)[:,0:2]).flatten())
            tango.put('sst', tmp)
            tango.end_transfer()

        elif self.rank == 1:
            tango = coupler.Tango(config, grid_name, 0, 4, 2, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tmp = np.array((send_sst.reshape(4, 4)[:,2:4]).flatten())
            tango.put('sst', tmp)
            tango.end_transfer()

        else:
            recv_sst = np.ones(len(send_sst), dtype='double')

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_sst)
            tango.end_transfer()

            assert(np.array_equal(send_sst, recv_sst))

        tango.finalize()


if __name__ == '__main__':
    try:
        unittest.main()
    except SystemExit:
        pass
