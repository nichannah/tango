cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(test)

find_package(GTest REQUIRED)
add_executable(test tango_test.cc)
include_directories(${GTEST_INCLUDE_DIRS})
target_link_libraries(test tango ${GTEST_LIBRARIES} ${MPI_CXX_LIBRARIES} -lpthread -lnetcdf_c++4 -lyaml-cpp)
