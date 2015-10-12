import os

try:
  include_paths = [os.environ['OMPI_ROOT'] + '/include/GNU',
                   os.environ['NETCDF_BASE'] + '/include',
                   os.environ['HOME'] + '/.local/include/']
except KeyError:
  include_paths = ['/usr/include']

env = Environment(ENV = os.environ, CXX='mpic++', CC='mpicc',
                  F90='mpif90', FORTRAN='mpif90',
                  CXXFLAGS=['-std=c++11', '-Wall', '-g', '-O0'],
                  CPPPATH=['#include'] + include_paths)
mods = SConscript('lib/SConscript', exports='env')
SConscript('test/SConscript', exports='env')
SConscript('bin/SConscript', exports='env')

def move_mod_files(target=None, source=None, env=None):
    for t in target:
        if t.name[-4:] == '.mod':
            shutil.move(t.name, str(t))
env.AddPostAction(mods, move_mod_files)
