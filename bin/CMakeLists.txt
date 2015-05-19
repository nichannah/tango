cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(python-mpi)

find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_PATH})
add_executable(python-mpi python-mpi.cc)
target_link_libraries(python-mpi ${PYTHON_LIBRARIES} ${MPI_CXX_LIBRARIES})
