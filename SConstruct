import os

compiler = 'GNU'

env = Environment(ENV = {'PATH' : os.environ['PATH']}, CXX='mpic++',
                  CCFLAGS='-std=c++0x',
                  CPPPATH=['#include', os.environ['FPATH_' + compiler], os.environ['NETCDF_BASE'] + '/include'])
SConscript('lib/SConscript', exports='env')
SConscript('test/SConscript', exports='env')
