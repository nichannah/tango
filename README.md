# tango

An Earth System Model coupler.

# Getting Started

```
$ sudo apt-get install libnetcdf-dev
$ sudo apt-get install libboost-dev (needed by libyaml)
$ sudo apt-get install libyaml-cpp-dev
$ sudo apt-get install libpython-dev
$ sudo apt-get install scons
```

Download and install netcdf-cxx4 from https://github.com/Unidata/netcdf-cxx4

Download and install libgtest-dev, e.g.:
```
$ sudo apt-get install libgtest-dev
$ cd /usr/src/gtest
$ sudo cmake .
$ sudo make
$ sudo mv libg* /usr/lib/
```

Build with:
```
$ scons
```

Run the tests:
```
$ export LD_LIBRARY_PATH=/usr/local/lib:$HOME/tango/lib:$LD_LIBRARY_PATH
$ export PYTHONPATH=$HOME/tango/lib:$PYTHONPATH
$ mpirun -n 2 ./bin/python-mpi test/test_basic_decomposition.py
$ cd test
$ mpirun -n 2 tango_test.exe
```

# Concepts

There are several key concepts that are needed to understand the source code.

Grid:

Mapping:

Tile:

Peer:

Transfer:

