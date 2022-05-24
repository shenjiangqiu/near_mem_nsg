project(nsg_bin)

add_executable(main main.cc)
target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main PUBLIC near_mem_nsg fmt::fmt efanna2e cpp_bloom)



add_executable(simulation simulation.cc)
target_include_directories(simulation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(simulation PUBLIC near_mem_nsg fmt::fmt efanna2e cpp_bloom)

