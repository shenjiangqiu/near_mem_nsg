project(cityhash)
aux_source_directory(src CITYHASH_SRC)
add_library(cityhash SHARED ${CITYHASH_SRC})
target_include_directories(cityhash PUBLIC src)