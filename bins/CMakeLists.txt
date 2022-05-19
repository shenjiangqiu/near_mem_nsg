project(nsg_bin)

add_executable(main main.cc)
target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main PUBLIC near_mem_nsg fmt::fmt)
