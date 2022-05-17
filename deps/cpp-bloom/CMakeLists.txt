project(cpp_bloom)
add_library(cpp_bloom INTERFACE)
target_include_directories(cpp_bloom INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/")
target_link_libraries(cpp_bloom INTERFACE cityhash)