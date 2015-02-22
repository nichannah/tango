
import ctypes as ct

lib = None;

def tango_init():
    lib = cdll.LoadLibrary('libtango.so')
    lib.tango_begin_transfer.argtypes = []
    lib.tango_put.argtypes = []
    lib.tango_get.argtypes = []

    lib.tango_init()

def tango_begin_transfer(time, grid_name):
    lib.tango_begin_transfer(time, grid_name.ctypes.data_as(ctypes.POINTER(ctypes.c_char)))

def tango_put(field_name, array, size):
    lib.tango_put(field_name.ctypes.data_as(ctypes.POINTER(ctypes.c_char)),
                  array.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), 
                  size)

def tango_get(field_name, array, size):
    lib.tango_get(field_name.ctypes.data_as(ctypes.POINTER(ctypes.c_char)),
                  array.ctypes.data_as(ctypes.POINTER(ctypes.c_double)), 
                  size)

def tango_finalize():

    lib.tango_finalize()
    lib = None

