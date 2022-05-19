
cmake_minimum_required(VERSION "3.12" )
project(near_mem_nsg VERSION 0.1)
#settings
set(CMAKE_CXX_STANDARD 20 )
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set -Wall
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
#deps
add_subdirectory(deps/fmt)
add_subdirectory(deps/plog)
add_subdirectory(deps/cityhash)
add_subdirectory(deps/nsg)
add_subdirectory(deps/cpp-bloom)
add_subdirectory(deps/Catch2)
add_subdirectory(deps/ramulator)
#main project
aux_source_directory(src MAINSOURCE)
add_library(near_mem_nsg ${MAINSOURCE})
target_link_libraries(near_mem_nsg PUBLIC fmt plog cityhash cpp_bloom ramulator)
target_include_directories(near_mem_nsg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

#tests
add_subdirectory(tests)

#binaries
add_subdirectory(bins)
