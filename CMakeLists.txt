
cmake_minimum_required(VERSION "3.12" )
project(near_mem_nsg VERSION 0.1)
#settings
set(CXX_STANDARD 20 )
set(CXX_STANDARD_REQUIRED ON)
#deps
add_subdirectory(deps/fmt)
add_subdirectory(deps/plog)
add_subdirectory(deps/cityhash)
add_subdirectory(deps/cpp-bloom)

#tests
add_subdirectory(tests)

