
from __future__ import print_function

import sys
import unittest
import os
import tango as coupler
import ctypes as ct
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


class TestInterpMethods(unittest.TestCase):
    """
    Test the different interpolation methods.
    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommrank.so')
        mpi.call_mpi_comm_rank.restype = ct.c_int
        self.rank = mpi.call_mpi_comm_rank()

    def run_test(self, test_name, config):

        if self.rank == 0:
            field = np.zeros((94, 192))
            field[:47, :] = 10

            tango = coupler.Tango(config, 'atm', 0, 192, 0, 94, 0, 192, 0, 94)
            tango.begin_transfer(0, 'ice')
            tango.put('u', field)
            tango.end_transfer()

            plt.imshow(field, origin='lower')
            plt.savefig('src_{}.png'.format(test_name))

        else:
            field = np.zeros((1080, 1440), dtype='float64')
            tango = coupler.Tango(config, 'ice', 0, 1440, 0, 1080, 0, 1440, 0, 1080)
            tango.begin_transfer(0, 'atm')
            tango.get('u', field)
            tango.end_transfer()

            plt.imshow(field, origin='lower')
            plt.savefig('dest_{}.png'.format(test_name))

        tango.finalize()

    def test_conserve(self):
        config = os.path.join(self.test_dir,
                              'test_input-interp_method-conserve')
        self.run_test('conserve', config)


    def test_bilinear(self):
        config = os.path.join(self.test_dir,
                              'test_input-interp_method-bilinear')
        self.run_test('bilinear', config)


    def test_patch(self):

        config = os.path.join(self.test_dir,
                              'test_input-interp_method-patch')
        self.run_test('patch', config)


if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a chance for MPI_Finalize() to be called.
    try:
        unittest.main()
    except SystemExit:
        pass
