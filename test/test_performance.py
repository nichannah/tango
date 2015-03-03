
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct
import numpy as np

class TestPerformance(unittest.TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommrank.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def test_one_way_send_receive(self):
        """
        Send a large single array between two large grids.
        """

        grid_name = 'ocean'

        send_array = np.arange(10000);

        config = os.path.join(self.test_dir, 'perf_input-1_mappings-2_grids')
        if self.rank == 0:
            tango = coupler.Tango(config, grid_name, 0, 100, 0, 100, 0, 100, 0, 100)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_array)
            tango.end_transfer()
        else:
            recv_array = np.zeros(len(send_array))

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 100, 0, 100, 0, 100, 0, 100)
            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_array)
            tango.end_transfer()

            #assert(np.array_equal(send_array, recv_array))
            print('Recv array {}'.format(recv_array))

        tango.finalize()

if __name__ == '__main__':
    try:
        unittest.main()
    except SystemExit:
        pass
