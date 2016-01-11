#!/bin/bash

module purge
module load scons
module load gcc/4.9.0
module load intel-fc
module load openmpi/1.8.4
module load netcdf
module load boost

scons