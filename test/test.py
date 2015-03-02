
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct

class TestInterface(unittest.TestCase):

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommrank.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def test_init(self):

        config = os.path.join(self.test_dir, 'test_input-1_mappings-2_grids')
        if self.rank == 0:
            tango = coupler.Tango(config, 'ocean', 0, 4, 0, 4, 0, 4, 0, 4)
        else:
            tango = coupler.Tango(config, 'ice', 0, 4, 0, 4, 0, 4, 0, 4)

        tango.finalize()

if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a change for MPI_Finalize() to be called. 
    try:
        unittest.main()
    except SystemExit:
        pass
