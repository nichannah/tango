cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(libtango)

add_library(tango SHARED tango.cc router.cc config.cc)
target_link_libraries(tango)
