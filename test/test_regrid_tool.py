
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

    def run_test(self, config):
        """
        Regrid a single field from CORE sized to MOM sized grid.
        """

        recv_u = np.zeros((1080, 1440), dtype='float64')

        if self.rank == 0:
            # Read in field to to regridded.
            with nc.Dataset(os.path.join(config, 'u_10.0001.nc')) as f:
                u = np.array(f.variables['U_10'][0,:,:], dtype='float64')

            tango = coupler.Tango(config, 'atm', 0, 192, 0, 94, 0, 192, 0, 94)
            tango.begin_transfer(0, 'ice')
            tango.put('u', u)
            tango.end_transfer()

        else:
            tango = coupler.Tango(config, 'ice', 0, 1440, 0, 1080, 0, 1440, 0, 1080)
            tango.begin_transfer(0, 'atm')
            tango.get('u', recv_u)
            tango.end_transfer()

        tango.finalize()
        return recv_u


    def test_2d_interp(self):
        """
        Interpolate a 2d field using three different methods and compare.
        """
        config = os.path.join(self.test_dir,
                              'test_input-regrid_tool-conserve')
        conserve_res = self.run_test(config)

        config = os.path.join(self.test_dir,
                              'test_input-regrid_tool-bilinear')
        bilinear_res = self.run_test(config)

        config = os.path.join(self.test_dir,
                              'test_input-regrid_tool-patch')
        patch_res = self.run_test(config)

        if self.rank == 1:
            plt.imshow(conserve_res, origin='lower')
            plt.savefig(os.path.join(self.test_dir, 'test_interp_conserve.png'))
            plt.close()
            plt.imshow(bilinear_res, origin='lower')
            plt.savefig(os.path.join(self.test_dir, 'test_interp_bilinear.png'))
            plt.close()
            plt.imshow(patch_res, origin='lower')
            plt.savefig(os.path.join(self.test_dir, 'test_interp_patch.png'))
            plt.close()

            assert(np.sum(conserve_res - bilinear_res) / np.sum(conserve_res) < 0.001)
            assert(np.sum(conserve_res - patch_res) / np.sum(conserve_res) < 0.1)

    def test_3d_interp(self):
        """
        This is not really 3d interpolation, but 2d on many levels.

        The test we use is to regrid from 0.25 to 1.0 deg to MOM grid.
        """




if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a chance for MPI_Finalize() to be called.
    try:
        unittest.main()
    except SystemExit:
        pass
