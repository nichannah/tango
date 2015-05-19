cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(test)

add_executable(test tango_test.cc)
target_link_libraries(test tango ${MPI_CXX_LIBRARIES})
