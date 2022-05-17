#target test1
add_executable(cpp20test cpp20test.cc)
target_link_libraries(cpp20test fmt plog cityhash cpp_bloom)

#target test2
add_executable(bloomtest cpp_bloom_test.cc)
target_link_libraries(bloomtest fmt plog cityhash cpp_bloom)
