
import ctypes as ct
import os
import numpy as np

class Tango:

    def __init__(self, config, grid, lis, lie, ljs, lje, gis, gie, gjs, gje):

        self.lib = ct.cdll.LoadLibrary('libtango.so')

        self.lib.tango_init.argtypes = [ct.POINTER(ct.c_char),
                                        ct.POINTER(ct.c_char),
                                        ct.c_uint, ct.c_uint, ct.c_uint,
                                        ct.c_uint, ct.c_uint, ct.c_uint,
                                        ct.c_uint, ct.c_uint]
        self.lib.tango_begin_transfer.argtypes = [ct.c_int,
                                                  ct.POINTER(ct.c_char)]
        self.lib.tango_put.argtypes = [ct.POINTER(ct.c_char),
                                       ct.POINTER(ct.c_double), ct.c_int]
        self.lib.tango_get.argtypes = [ct.POINTER(ct.c_char),
                                       ct.POINTER(ct.c_double), ct.c_int]

        self.lib.tango_init(config, grid, lis, lie, ljs, lje,
                                               gis, gie, gjs, gje)

    def begin_transfer(self, time, grid_name):
        self.lib.tango_begin_transfer(time, grid_name)

    def put(self, field_name, array):
        tmp = np.ascontiguousarray(array)
        self.lib.tango_put(field_name,
                           tmp.ctypes.data_as(ct.POINTER(ct.c_double)),
                           tmp.size)

    def get(self, field_name, array):
        tmp = np.ascontiguousarray(array)
        self.lib.tango_get(field_name,
                           tmp.ctypes.data_as(ct.POINTER(ct.c_double)),
                           tmp.size)

    def end_transfer(self):
        self.lib.tango_end_transfer()

    def finalize(self):
        self.lib.tango_finalize()
        self.lib = None
