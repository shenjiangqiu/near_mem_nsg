
cmake_minimum_required(VERSION 3.10)
project(ramulator)
set(CMAKE_CXX_FLAGS "-Wall" )
set(CMAKE_CXX_STANDARD 11)
aux_source_directory(src source)
add_library(ramulator ${source})
target_include_directories(ramulator PUBLIC src)

add_executable(ramulator_test main.cc)
target_link_libraries(ramulator_test PUBLIC ramulator)
install (TARGETS ramulator DESTINATION .)
