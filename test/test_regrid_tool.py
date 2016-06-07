
from __future__ import print_function

import sys
import unittest
import os
import tempfile
import shlex
import shutil
import subprocess as sp
import netCDF4 as nc
import ctypes as ct
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../lib'))
import tango as coupler

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../scrip_grids'))
import mom2scrip
import square2scrip

class TestRegrid(unittest.TestCase):
    """
    Test using tango as a stand-alone regridding tool.

    The upside of this is a single approach to do all regridding either
    stand-alone or within the model.
    Downsides: - it depends on MPI.
               - it cannot (easily) do 3d fields.

    These tests should be called from tango top-level dir with:
        export PYTHONPATH=$HOME/more_home/tango/lib:$PYTHONPATH
        mpirun -n 2 ./bin/python-mpi test/test_regrid_tool.py

    """

    def setUp(self):
        self.test_dir = os.path.dirname(os.path.realpath(__file__))
        mpi = ct.cdll.LoadLibrary('test/libmpicommbasic.so')
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


    def _test_2d_interp_small_to_big(self):
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

    def _test_2d_interp_big_to_small(self):
        """
        Interpolate a 2d field from big to small.
        """

        config = os.path.join(self.test_dir, 'test_input-regrid_tool-2d')

        if self.rank == 0:
            # Read in field to to regridded.
            with nc.Dataset(os.path.join(config, 'temp.nc')) as f:
                temp = np.array(f.variables['temp'][0,:,:], dtype='float64')

            tango = coupler.Tango(config, 'ice', 0, 1080, 0, 1440, 0, 1080, 0, 1440)
            tango.begin_transfer(0, 'atm')
            tango.put('temp', temp)
            tango.end_transfer()

        else:
            recv_temp = np.zeros((300, 360), dtype='float64')
            tango = coupler.Tango(config, 'atm', 0, 300, 0, 360, 0, 300, 0, 360)
            tango.begin_transfer(0, 'ice')
            tango.get('temp', recv_temp)
            tango.end_transfer()

            plt.imshow(recv_temp, origin='lower')
            plt.savefig(os.path.join(self.test_dir, 'test_interp_2d.png'))
            plt.close()

        tango.finalize()


    def _test_3d_interp(self):
        """
        This is not really 3d interpolation, but 2d on many levels.

        The test we use is to regrid from 0.25 to 1.0 deg to MOM grid.
        """

        config = os.path.join(self.test_dir, 'test_input-regrid_tool-3d')

        if self.rank == 0:
            input = os.path.join(config, 'ocean_1440x1080_temp_salt.res.nc')
            f = nc.Dataset(input)

            # Read in field to to regridded.
            tango = coupler.Tango(config, 'ice', 0, 1440, 0, 1080,
                                                 0, 1440, 0, 1080)
            print('Finished tango init on rank {}'.format(self.rank))

            for var in ['salt', 'temp']:
                for l in range(f.variables[var].shape[0]):
                    data = np.array(f.variables[var][l,:,:], dtype='float64')

                    print('Send field {}'.format(l))
                    tango.begin_transfer(l, 'ocn')
                    tango.put('data', data)
                    tango.end_transfer()

            f.close()

        else:
            input = os.path.join(config, 'ocean_1440x1080_temp_salt.res.nc')
            output = os.path.join(config, 'ocean_360x300_temp_salt.res.nc')
            f_in = nc.Dataset(input, 'r')

            # Create the output, including copying over any attributes from the
            # input.
            f_out = nc.Dataset(output, 'w')
            f_out.createDimension('GRID_X_T', 360)
            f_out.createDimension('GRID_Y_T', 300)
            f_out.createDimension('ZT', 50)
            vs = f_out.createVariable('salt', 'f8', ('ZT', 'GRID_Y_T', 'GRID_X_T'),
                                      fill_value = f_in.variables['salt']._FillValue)
            for attr_name in f_in.variables['salt'].ncattrs():
                if attr_name == '_FillValue':
                    continue

                attr_val = f_in.variables['salt'].getncattr(attr_name)
                vs.setncattr(attr_name, attr_val)

            vt = f_out.createVariable('temp', 'f8', ('ZT', 'GRID_Y_T', 'GRID_X_T'),
                                      fill_value = f_in.variables['temp']._FillValue)
            for attr_name in f_in.variables['temp'].ncattrs():
                if attr_name == '_FillValue':
                    continue

                attr_val = f_in.variables['temp'].getncattr(attr_name)
                vs.setncattr(attr_name, attr_val)

            tango = coupler.Tango(config, 'ocn', 0, 360, 0, 300,
                                                 0, 360, 0, 300)
            print('Finished tango init on rank {}'.format(self.rank))
            recv_array = np.zeros((300, 360), dtype='float64')

            for var in ['salt', 'temp']:
                for l in range(f_out.variables[var].shape[0]):
                    tango.begin_transfer(l, 'ice')
                    tango.get('data', recv_array)
                    tango.end_transfer()
                    print('Received regridded field {}'.format(l))

                    f_out.variables[var][l, :, :] = recv_array

            f_out.close()
            f_in.close()

        tango.finalize()

    #def make_MOM5_SCRIP_grid(self):
    #    """
    #    Make a SCRIP definiton of the MOM5 tri-polar grid.
#
#        Returns the path to the grid file.
#        """
#
#        grid_path = '/short/v45/nah599/mom/input/gfdl_nyf_1080/ocean_hgrid.nc'
#        (_, scrip_path) = tempfile.mkstemp()
#
#        grid = mom2scrip.MomGrid(grid_path)
#        grid.to_scrip('t', scrip_path, '')
#
#        return scrip_path
#
#    def make_square_SCRIP_grid(self, num_lats, num_lons):
#        """
#        Make a SCRIP definiton of a regular square grid.
#
#        Returns the path to the grid file.
#        """
#
#        grid = square2scrip.LatLonGrid(num_lats, num_lons, None)
#        (_, scrip_path) = tempfile.mkstemp()
#
#        grid.write('', scrip_path)
#
#        return scrip_path

#    def make_remap_grid(self, src_grid, dest_grid, output):
#        """
#        Make a remap grid from src and destination grids.
#        """
#
#        cmd = 'ESMF_RegridWeightGen -s {} -d {} -w {} -m conserve'.format(src_grid, dest_grid, output)
#        sp.call(shlex.split(cmd))

    def test_HadISST_to_MOM5(self):
        """
        Remap an SST trend file to MOM5 grid.
        """

        config = os.path.join(self.test_dir, 'regrid_tool-HadISST_to_MOM5')

        if self.rank == 0:
            # This is the source, read in the field to be regridded.
            with nc.Dataset(os.path.join(config, 'SSTTrend_1992to2011.nc')) as f:
                # Need to flip because this input is strange.
                data = np.array(np.flipud(f.variables['SST'][:]), dtype='float64')

            tango = coupler.Tango(config, 'src', 0, 360, 0, 180,
                                                 0, 360, 0, 180)
            tango.begin_transfer(1, 'dest')
            tango.put('data', data)
            tango.end_transfer()

        elif self.rank == 1:
            # This is the destination, create ouput file containing the regridded field.
            data = np.zeros((1080, 1440), dtype='float64')

            tango = coupler.Tango(config, 'dest', 0, 1440, 0, 1080,
                                                 0, 1440, 0, 1080)
            tango.begin_transfer(1, 'src')
            tango.get('data', data)
            tango.end_transfer()

            output = os.path.join(config, 'SSTTrend_1992to2011_mom_grid.nc')
            f_out = nc.Dataset(output, 'w')
            grid = nc.Dataset('http://ec2-52-25-62-253.us-west-2.compute.amazonaws.com/ocean_grid.nc')

            f_out.createDimension('yt_ocean', 1080)
            yt = f_out.createVariable('yt_ocean', 'f8', ('yt_ocean'))
            yt_grid = grid.variables['yt_ocean']
            yt.setncatts({k: str(yt_grid.getncattr(k)) for k in yt_grid.ncattrs()})

            f_out.createDimension('xt_ocean', 1440)
            xt = f_out.createVariable('xt_ocean', 'f8', ('xt_ocean'))
            xt_grid = grid.variables['xt_ocean']
            xt.setncatts({k: str(xt_grid.getncattr(k)) for k in xt_grid.ncattrs()})
            xt[:] = xt_grid[:]

            f_out.createDimension('st_ocean', 50)
            st = f_out.createVariable('st_ocean', 'f8', ('st_ocean'))
            st_grid = grid.variables['st_ocean']
            st.setncatts({k: str(st_grid.getncattr(k)) for k in st_grid.ncattrs()})
            st[:] = st_grid[:]

            f_out.createDimension('time')
            time = f_out.createVariable('time', 'f8', ('time'))
            time.long_name = 'observation time'
            time.units = 'days since 2001-01-01 00:00:00'
            time.calendar = 'noleap'
            time[:] = 0.0
            var = f_out.createVariable('temp', 'f8', ('time', 'st_ocean', 'yt_ocean', 'xt_ocean'))

            var.long_name = 'Temperature'
            var[0, 0, :, :] = data[:, :]
            var[0, 1:, :, :] = 0.0

            f_out.close()


if __name__ == '__main__':
    # This interpreter doesn't like exit() being called, because it doesn't
    # leave a chance for MPI_Finalize() to be called.
    try:
        unittest.main()
    except SystemExit:
        pass
