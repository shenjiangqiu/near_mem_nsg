aux_source_directory(./ TESTS)
add_executable(all_tests ${TESTS})
target_link_libraries(all_tests PRIVATE Catch2::Catch2WithMain cpp_bloom plog fmt ramulator near_mem_nsg)