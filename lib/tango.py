
import ctypes as ct
import os
import resource
import numpy as np

class Tango:

    def __init__(self, config, grid, lis, lie, ljs, lje, gis, gie, gjs, gje):

        # FIXME: this doesn't appear to work.
        resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY,
                                                   resource.RLIM_INFINITY))

        my_dir = os.path.dirname(os.path.realpath(__file__))
        build_dir = os.path.join(my_dir, '../', 'build')
        self.lib = ct.cdll.LoadLibrary(os.path.join(build_dir, 'libtango.so'))

        self.lib.tango_init.argtypes = [ct.c_char_p, ct.c_char_p,
                                        ct.c_uint, ct.c_uint, ct.c_uint,
                                        ct.c_uint, ct.c_uint, ct.c_uint,
                                        ct.c_uint, ct.c_uint]
        self.lib.tango_begin_transfer.argtypes = [ct.c_char_p, ct.c_char_p]
        self.lib.tango_put.argtypes = [ct.c_char_p,
                                       ct.POINTER(ct.c_double), ct.c_int]
        self.lib.tango_get.argtypes = [ct.c_char_p,
                                       ct.POINTER(ct.c_double), ct.c_int]

        self.lib.tango_init(config.encode('ascii'), grid.encode('ascii'),
                            lis, lie, ljs, lje, gis, gie, gjs, gje)

    def begin_transfer(self, timestamp, grid_name):
        self.lib.tango_begin_transfer(timestamp.encode('ascii'),
                                      grid_name.encode('ascii'))

    def put(self, field_name, array):
        assert(array.flags['C_CONTIGUOUS'])
        assert(array.dtype == 'float64')
        self.lib.tango_put(field_name.encode('ascii'),
                           array.ctypes.data_as(ct.POINTER(ct.c_double)),
                           array.size)

    def get(self, field_name, array):
        assert(array.flags['C_CONTIGUOUS'])
        assert(array.dtype == 'float64')
        self.lib.tango_get(field_name.encode('ascii'),
                           array.ctypes.data_as(ct.POINTER(ct.c_double)),
                           array.size)

    def end_transfer(self):
        self.lib.tango_end_transfer()

    def finalize(self):
        self.lib.tango_finalize()
        self.lib = None
