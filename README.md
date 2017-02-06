# tango

An Earth System Model coupler.

This is a new coupler, it is intended to _very_ _very_ easy to use and understand. Some good things about it:

* C/C++, Python and Fortran interfaces. This means that you can spec out your coupling scheme quickly using Python, or couple together models written in different languages.
* Regridding weights are made offline using ESMF, this can be run in parallel - essential for large grids.
* Fully distributed, i.e. N-to-N coupling of independent MPI processes.
* Excellent performance, all fields between any two models are bundled together and then sent, rather than each field going through MPI individually.
* Can be used as a general purpose regridding tool offline. The good thing about this is that you can explore the interpolation schemes without actually having to run the model.
* About 1000 lines of code, so if something goes wrong you have a hope of figuring it what's happening.
* The configuration is trivial.

# Getting started

Tango is in development. It uses libraries extensively in an attempt to reduce the amount of source code. This has worked well, the core library is around 1000 lines of code (LOC). However 

```
$ sudo apt-get install libnetcdf-c++4-dev
$ sudo apt-get install libboost-dev (for now, newer versions of libyaml-cpp will not need this).
$ sudo apt-get install libyaml-cpp-dev
$ sudo apt-get install libpython-dev
```

Download and install [ESMF](https://www.earthsystemcog.org/projects/esmf/), in particular the ESMF_RegridWeightGen program is needed.

tango can then be built with:
```
$ make
```

To run the tests:
```
$ export LD_LIBRARY_PATH=/usr/local/lib:$HOME/tango/lib:$LD_LIBRARY_PATH
$ export PYTHONPATH=$HOME/more_home/tango/lib:$PYTHONPATH
$ mpirun -n 2 ./bin/python-mpi test/test_basic_decomposition.py
$ cd test
$ mpirun -n 2 tango_test.exe
```

# Running offline

Tango can be used to regrid fields offline. There is a test case that does this. 

# Concepts

There are several key concepts that are needed to understand the source code.

Grid:

Mapping:

Tile:

Peer:

Transfer:

