#!/usr/bin/env python

from __future__ import print_function

import sys
import argparse
import numpy as np
import netCDF4 as nc

"""
Create a NetCDF grid definition file in SCRIP format. The grid created is a
standard lat-lon grid.
"""

class LatLonGrid:
    """
    """

    def __init__(self, num_lon_points, num_lat_points, mask=None):

        self.num_lon_points = num_lon_points
        self.num_lat_points = num_lat_points
        self.corners = 4

        # Set lats and lons.
        self.lon = np.linspace(0, 2*np.pi, num_lon_points, endpoint=False)
        self.lat = np.linspace(-np.pi / 2, np.pi / 2, num_lat_points)
        dx_half = 2*np.pi / num_lon_points / 2.0
        dy_half = np.pi / num_lat_points / 2.0

        # Similar to lon, lat but specify the coordinate at every grid
        # point. Also it wraps along longitude.
        self.x = np.tile(self.lon, (num_lat_points, 1))
        self.y = np.tile(self.lat, (num_lon_points, 1))
        self.y = self.y.transpose()

        def make_corners(x, y, dx, dy):

            # Set grid corners, we do these one corner at a time. Start at the 
            # bottom left and go anti-clockwise. This is the SCRIP convention.
            clon = np.empty((self.corners, x.shape[0], x.shape[1]))
            clon[:] = np.NAN
            clon[0,:,:] = x - dx
            clon[1,:,:] = x + dx
            clon[2,:,:] = x + dx
            clon[3,:,:] = x - dx
            assert(not np.isnan(np.sum(clon)))

            clat = np.empty((self.corners, x.shape[0], x.shape[1]))
            clat[:] = np.NAN
            clat[0,:,:] = y[:,:] - dy
            clat[1,:,:] = y[:,:] - dy
            clat[2,:,:] = y[:,:] + dy
            clat[3,:,:] = y[:,:] + dy

            # The bottom latitude band should always be Southern extent, for
            # all t, u, v.
            clat[0, 0, :] = -np.pi / 2
            clat[1, 0, :] = -np.pi / 2

            # The top latitude band should always be Northern extent, for all
            # t, u, v.
            clat[2, -1, :] = np.pi / 2
            clat[3, -1, :] = np.pi / 2

            assert(not np.isnan(np.sum(clat)))

            return clon, clat

        self.clon, self.clat = make_corners(self.x, self.y, dx_half, dy_half)

        import pdb
        pdb.set_trace()


    def write(self, command, output):
        """
        Create the netcdf file and write the output.
        """

        f = nc.Dataset(output, 'w+')
        f.createDimension('grid_size', FIXME)
        f.createDimension('grid_corners', 4)
        f.createDimension('grid_rank', 2)

        f.createVariable('grid_dims', 'i4', ('grid_rank'))
        center_lat = f.createVariable('grid_center_lat', 'f8', ('grid_size'))
        center_lat.units = 'radians'
        center_lon = f.createVariable('grid_center_lon', 'f8', ('grid_size'))
        center_lon.units = 'radians'
        imask = f.createVariable('grid_imask', 'i4', ('grid_size'))
        imask.units = 'unitless'
        corner_lat = f.createVariable('grid_corner_lat', 'f8', ('grid_size', 'grid_corners'))
        corner_lat.units = 'radians'
        corner_lon = f.createVariable('grid_corner_lon', 'f8', ('grid_size', 'grid_corners'))
        corner_lon.units = 'radians'

        f.title = '{}x{} rectangular grid'.format()
        f.history = command

        if mask is not None:
            pass

        f.close()


def main():


    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="The output file name.")
    parser.add_argument("--lon_points", default=1, help="""
                        The number of longitude points.
                        """)
    parser.add_argument("--lat_points", default=1, help="""
                        The number of latitude points.
                        """)
    parser.add_argument("--mask", default=None, help="The mask file name.")
    args = parser.parse_args()

    mask = None
    if args.mask is not None:
        # Read in the mask.
        pass

    grid = LatLonGrid(args.lon_points, args.lat_points, mask)
    grid.write(' '.join(sys.argv), args.output)

    return 0

if __name__ == "__main__":
    sys.exit(main())
