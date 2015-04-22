
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct
import numpy as np

# FIXME: remove these. Just use auto-generated.
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

class TestBasicDecomposition(unittest.TestCase):
    """
    Tests with only one tile per grid.

    These tests should be called with:
        mpirun -n 2 ./bin/python-mpi test/test_basic_decomposition.py
    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommbasic.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def test_init(self):
        """
        Most basic test to call init() and finalize().
        """

        config = os.path.join(self.test_dir,
                              'test_input-2_mappings-2_grids-4x4_to_4x4')
        if self.rank == 0:
            tango = coupler.Tango(config, 'ocean', 0, 4, 0, 4, 0, 4, 0, 4)
        else:
            tango = coupler.Tango(config, 'ice', 0, 4, 0, 4, 0, 4, 0, 4)

        tango.finalize()

    def test_one_way_send_receive(self):
        """
        Send a single array.
        """

        grid_name = 'ocean'

        config = os.path.join(self.test_dir,
                              'test_input-1_mappings-2_grids-4x4_to_4x4')
        if self.rank == 0:
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_sst)
            tango.end_transfer()
        else:
            recv_sst = np.zeros(len(send_sst))

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_sst)
            tango.end_transfer()

            assert(np.array_equal(send_sst, recv_sst))

        tango.finalize()


    def test_different_sized_grids(self):
        """
        Send a single array between different sized grids.
        """

        send_array = np.zeros((4, 4))
        send_array[2, 3] = 50.0
        #send_array[:, 1] = 1.0
        #send_array[:, 2] = 50.0
        #send_array[:, 3] = 4.0

        #config = os.path.join(self.test_dir,
        #                      'test_input-1_mappings-2_grids-4x4_to_8x8')
        #config = os.path.join(self.test_dir,
        #                      'test_input-1_mappings-2_grids-4x4_to_8x8-bilinear')
        config = os.path.join(self.test_dir,
                              'test_input-1_mappings-2_grids-4x4_to_8x8-patch')

        if self.rank == 0:
            tango = coupler.Tango(config, 'ice', 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ocean')
            tango.put('temp', send_array)
            tango.end_transfer()
        else:
            recv_array = np.zeros((8, 8))

            tango = coupler.Tango(config, 'ocean', 0, 8, 0, 8, 0, 8, 0, 8)
            tango.begin_transfer(0, 'ice')
            tango.get('temp', recv_array)
            tango.end_transfer()

            print('send_array {}'.format(send_array))
            print('recv_array {}'.format(recv_array))

            area_ratio = recv_array.size / send_array.size
            np.testing.assert_almost_equal(np.sum(recv_array),
                                           np.sum(send_array) * area_ratio)
            np.testing.assert_almost_equal(np.mean(send_array[0, :]),
                                           np.mean(recv_array[0, :]))
            np.testing.assert_almost_equal(np.mean(send_array[-1, :]),
                                           np.mean(recv_array[-1, :]))

        tango.finalize()


    def test_multiple_variable_send_receive(self):
        """
        Send multiple arrays.
        """

        grid_name = 'ocean'
        config = os.path.join(self.test_dir,
                              'test_input-1_mappings-2_grids-4x4_to_4x4')
        if self.rank == 0:
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_sst)
            tango.put('sss', send_sss)
            tango.end_transfer()
        else:
            recv_sst = np.zeros(len(send_sst))
            recv_sss = np.zeros(len(send_sss))

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_sst)
            tango.get('sss', recv_sss)
            tango.end_transfer()

            assert(np.array_equal(send_sst, recv_sst))
            assert(np.array_equal(send_sss, recv_sss))

        tango.finalize()

    def test_bidirectional_send_receive(self):
        """
        Send/recv an array in both directions.
        """

        grid_name = 'ocean'
        config = os.path.join(self.test_dir,
                              'test_input-2_mappings-2_grids-4x4_to_4x4')
        if self.rank == 0:
            recv_temp = np.zeros(len(send_temp))

            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)
            tango.begin_transfer(0, 'ice')
            tango.put('sst', send_sst)
            tango.put('sss', send_sss)
            tango.end_transfer()

            tango.begin_transfer(0, 'ice')
            tango.get('temp', recv_temp)
            tango.end_transfer()

            assert(np.array_equal(send_temp, recv_temp))

        else:
            recv_sst = np.zeros(len(send_sst))
            recv_sss = np.zeros(len(send_sss))

            grid_name = 'ice'
            tango = coupler.Tango(config, grid_name, 0, 4, 0, 4, 0, 4, 0, 4)

            tango.begin_transfer(0, 'ocean')
            tango.get('sst', recv_sst)
            tango.get('sss', recv_sss)
            tango.end_transfer()

            tango.begin_transfer(0, 'ocean')
            tango.put('temp', send_temp)
            tango.end_transfer()

            assert(np.array_equal(send_sst, recv_sst))
            assert(np.array_equal(send_sss, recv_sss))

        tango.finalize()


if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a change for MPI_Finalize() to be called. 
    try:
        unittest.main()
    except SystemExit:
        pass
