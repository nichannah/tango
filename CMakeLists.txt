cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(tango)

# Set GTest location for this machine
SET(ENV{GTEST_ROOT} $ENV{HOME}/.local)

find_package(MPI REQUIRED)
find_package(GTest REQUIRED)
# FIXME: get this working
# find_package(NetCDF REQUIRED)
SET(CMAKE_CXX_FLAGS "-std=c++11")
include_directories(${MPI_CXX_INCLUDE_PATH} include $ENV{NETCDF}/local)

add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(test)
