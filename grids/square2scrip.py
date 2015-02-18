#!/usr/bin/env python

from __future__ import print_function

import sys
import argparse
import numpy as np
import netCDF4 as nc

"""
Create a NetCDF grid definition file in SCRIP format. The grid created is a
uniform lat-lon grid.
"""

class LatLonGrid:
    """
    """

    def __init__(self, num_lon_points, num_lat_points, mask=None):

        self.num_lon_points = num_lon_points
        self.num_lat_points = num_lat_points
        self.corners = 4
        self.mask = mask;

        if mask is None:
            self.mask = np.ones((num_lon_points, num_lat_points))

        dx = 360.0 / num_lon_points
        dy = 180.0 / num_lat_points
        dx_half = dx / 2
        dy_half = dy / 2

        # Set lats and lons.
        self.lon = np.linspace(0, 360, num_lon_points, endpoint=False)
        # lat points exclude the poles.
        self.lat = np.linspace(-90 + dy_half, 90 - dy_half, num_lat_points)

        # Similar to lon, lat but specify the coordinate at every grid
        # point. Also it wraps along longitude.
        self.x = np.tile(self.lon, (num_lat_points, 1))
        self.y = np.tile(self.lat, (num_lon_points, 1))
        self.y = self.y.transpose()

        def make_corners(x, y, dx, dy):

            # Set grid corners, we do these one corner at a time. Start at the 
            # bottom left and go anti-clockwise. This is the SCRIP convention.
            clon = np.empty((x.shape[0], x.shape[1], self.corners))
            clon[:] = np.NAN
            clon[:,:,0] = x - dx
            clon[:,:,1] = x + dx
            clon[:,:,2] = x + dx
            clon[:,:,3] = x - dx
            assert(not np.isnan(np.sum(clon)))

            clat = np.empty((x.shape[0], x.shape[1], self.corners))
            clat[:] = np.NAN
            clat[:,:,0] = y - dy
            clat[:,:,1] = y - dy
            clat[:,:,2] = y + dy
            clat[:,:,3] = y + dy
            assert(not np.isnan(np.sum(clat)))

            # The bottom latitude band should always be Southern extent.
            assert(np.all(clat[0, :, 0] == -90))
            assert(np.all(clat[0, :, 1] == -90))

            # The top latitude band should always be Northern extent.
            assert(np.all(clat[-1, :, 2] == 90))
            assert(np.all(clat[-1, :, 3] == 90))

            return clon, clat

        self.clon, self.clat = make_corners(self.x, self.y, dx_half, dy_half)

    def write(self, command, output):
        """
        Create the netcdf file and write the output.
        """

        f = nc.Dataset(output, 'w')
        f.createDimension('grid_size',
                          self.num_lon_points * self.num_lat_points)
        f.createDimension('grid_corners', 4)
        f.createDimension('grid_rank', 2)

        grid_dims = f.createVariable('grid_dims', 'i4', ('grid_rank'))
        grid_dims[:] = [self.num_lon_points, self.num_lat_points]

        center_lat = f.createVariable('grid_center_lat', 'f8', ('grid_size'))
        center_lat.units = 'degrees'
        center_lat[:] = self.y[:].flatten()

        center_lon = f.createVariable('grid_center_lon', 'f8', ('grid_size'))
        center_lon.units = 'degrees'
        center_lon[:] = self.x[:].flatten()

        imask = f.createVariable('grid_imask', 'i4', ('grid_size'))
        imask.units = 'unitless'
        imask[:] = self.mask[:].flatten()

        corner_lat = f.createVariable('grid_corner_lat', 'f8',
                                      ('grid_size', 'grid_corners'))
        corner_lat.units = 'degrees'
        corner_lat[:] = self.clat[:].flatten()

        corner_lon = f.createVariable('grid_corner_lon', 'f8',
                                      ('grid_size', 'grid_corners'))
        corner_lon.units = 'degrees'
        corner_lon[:] = self.clon[:].flatten()

        f.title = '{}x{} rectangular grid'.format(self.num_lon_points,
                                                  self.num_lat_points)
        f.history = command
        f.close()


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="The output file name.")
    parser.add_argument("--lon_points", default=1, type=int, help="""
                        The number of longitude points.
                        """)
    parser.add_argument("--lat_points", default=1, type=int, help="""
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
