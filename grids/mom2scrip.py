#!/usr/bin/env python

from __future__ import print_function

import sys
import argparse
import numpy as np
import netCDF4 as nc


"""
Convert the mom grid definition to SCRIP format.
"""

class MomGrid:

    def __init__(self, grid_def, mask=None):

        with nc.Dataset(grid_def) as f:

            # Select points from double density grid.
            self.x_t = f.variables['x'][1::2,1::2]
            self.y_t = f.variables['y'][1::2,1::2]

            # Southern most U points are excluded. also the last (Eastern) U
            # points, they are duplicates of the first.
            self.x_u = f.variables['x'][2::2,0:-1:2]
            self.y_u = f.variables['y'][2::2,0:-1:2]

            x = f.variables['x'][:]
            y = f.variables['y'][:]

        # Corners of t points. Index 0 is bottom left and then
        # anti-clockwise.
        self.clon_t = np.empty((self.x_t.shape[0], self.x_t.shape[1], 4))
        self.clon_t[:] = np.NAN
        self.clon_t[:,:,0] = x[0:-1:2,0:-1:2]
        self.clon_t[:,:,1] = x[0:-1:2,2::2]
        self.clon_t[:,:,2] = x[2::2,2::2]
        self.clon_t[:,:,3] = x[2::2,0:-1:2]
        assert(not np.isnan(np.sum(self.clon_t)))

        self.clat_t = np.empty((self.x_t.shape[0], self.x_t.shape[1], 4))
        self.clat_t[:] = np.NAN
        self.clat_t[:,:,0] = y[0:-1:2,0:-1:2]
        self.clat_t[:,:,1] = y[0:-1:2,2::2]
        self.clat_t[:,:,2] = y[2::2,2::2]
        self.clat_t[:,:,3] = y[2::2,0:-1:2]
        assert(not np.isnan(np.sum(self.clat_t)))


        # Corners of u points.
        # FIXME: do these.

        if mask is None:
            self.mask = np.zeros((self.x_t.shape[0], self.x_t.shape[1]), dtype=bool)
        else:
            with nc.Dataset(mask) as f:
                self.mask = f.variables['mask'][:]


    def make_scrip(self, f, x, y, clon, clat):

        f.createDimension('grid_size', x.shape[0] * x.shape[1])
        f.createDimension('grid_corners', 4)
        f.createDimension('grid_rank', 2)

        grid_dims = f.createVariable('grid_dims', 'i4', ('grid_rank'))
        grid_dims[:] = [x.shape[1], x.shape[0]]

        center_lat = f.createVariable('grid_center_lat', 'f8', ('grid_size'))
        center_lat.units = 'degrees'
        center_lat[:] = y[:].flatten()

        center_lon = f.createVariable('grid_center_lon', 'f8', ('grid_size'))
        center_lon.units = 'degrees'
        center_lon[:] = x[:].flatten()

        imask = f.createVariable('grid_imask', 'i4', ('grid_size'))
        imask.units = 'unitless'
        # Invert the mask. SCRIP uses zero for points that do not participate.
        imask[:] = np.invert(self.mask[:]).flatten()

        corner_lat = f.createVariable('grid_corner_lat', 'f8',
                                      ('grid_size', 'grid_corners'))
        corner_lat.units = 'degrees'
        corner_lat[:] = clat[:].flatten()

        corner_lon = f.createVariable('grid_corner_lon', 'f8',
                                      ('grid_size', 'grid_corners'))
        corner_lon.units = 'degrees'
        corner_lon[:] = clon[:].flatten()


    def to_scrip(self, type, output, command):

        f = nc.Dataset(output, 'w')

        if type == 't':
            self.make_scrip(f, self.x_t, self.x_t, self.clon_t, self.clat_t)
        else:
            self.make_scrip(f, self.x_u, self.x_u, self.clon_u, self.clat_u)

        f.title = 'MOM tripolar {}-cell grid'.format(type)
        f.history = command
        f.close()


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="""
                        The input file name, must be a MOM grid definition
                        file.""")
    parser.add_argument("output", help="The output file name.")
    parser.add_argument("--type", default='t',
                         help="""Whether to use MOM t or u points to make the
                         SCRIP grid.""")

    args = parser.parse_args()

    # U grid not supported yet.
    assert(args.type == 't')

    grid = MomGrid(args.input)
    grid.to_scrip(args.type, args.output, ' '.join(sys.argv))

    return 0

if __name__ == '__main__':
    sys.exit(main())
