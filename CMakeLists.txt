cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(tango)

find_package(MPI REQUIRED)
SET(CMAKE_CXX_FLAGS "-std=c++11")
include_directories(${MPI_CXX_INCLUDE_PATH} include)

add_subdirectory(lib)
add_subdirectory(bin)
add_subdirectory(test)
