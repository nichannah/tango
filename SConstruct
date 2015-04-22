import os

try:
  include_paths = [os.environ['OMPI_ROOT'] + '/include/GNU',
                   os.environ['NETCDF_BASE'] + '/include',
                   os.environ['HOME'] + '/.local/include/']
except KeyError:
  include_paths = ['/usr/include']

env = Environment(ENV = os.environ, CXX='mpic++', CC='mpicc',
                  CXXFLAGS=['-std=c++11', '-Wall', '-g', '-O0'],
                  CPPPATH=['#include'] + include_paths)
SConscript('lib/SConscript', exports='env')
SConscript('test/SConscript', exports='env')
SConscript('bin/SConscript', exports='env')
