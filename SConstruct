import os

include_paths = [os.environ['OMPI_ROOT'] + '/include/GNU',
                 os.environ['NETCDF_BASE'] + '/include',
                 os.environ['HOME'] + '/.local/include/']

env = Environment(ENV = os.environ, CXX='mpic++',
                  CCFLAGS='-std=c++0x',
                  CPPPATH=['#include'] + include_paths)
SConscript('lib/SConscript', exports='env')
SConscript('test/SConscript', exports='env')
