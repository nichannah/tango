#!/usr/bin/env python

from __future__ import print_function

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
        self.lon = np.linspace(0, 360, num_lon_points, endpoint=False)
        self.lat = np.linspace(-90, 90, num_lat_points)
        dx_half = 360.0 / num_lon_points / 2.0
        dy_half = (180.0 / (num_lat_points - 1) / 2.0)

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
            clat[0, 0, :] = -90
            clat[1, 0, :] = -90

            # The top latitude band should always be Northern extent, for all
            # t, u, v.
            clat[2, -1, :] = 90
            clat[3, -1, :] = 90

            assert(not np.isnan(np.sum(clat)))

            return clon, clat

        self.clon, self.clat = make_corners(self.x, self.y, dx_half, dy_half)


    def write(self, output):
        """
        Create the netcdf file and write the output.
        """

        if mask is None:
            # Write




def main():

    args = parser.parse_args()

    parser.add_argument("output", help="The output file name.")
    parser.add_argument("--lon_points", default=192, help="""
                        The number of longitude points.
                        """)
    parser.add_argument("--lat_points", default=145, help="""
                        The number of latitude points.
                        """)
    parser.add_argument("--mask", default=None, help="The mask file name.")

    mask = None
    if args.mask is not None:
        # Read in the mask.
        pass

    grid = LatLonGrid(args.lon_points, args.lat_points, mask)
    grid.write(args.output)

    return 0

if __name__ == "__main__":
    sys.exit(main())
