set(CMAKE_CXX_STANDARD 11)

file(GLOB_RECURSE CPP_SOURCES *.cpp)
# find_package(Boost REQUIRED)
add_library(${PROJECT_NAME} ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(${PROJECT_NAME} cpp_bloom)
target_compile_options(${PROJECT_NAME} PUBLIC "-march=native" "-Wall" "-fno-builtin-mallloc" "-fno-builtin-calloc" "-fno-builtin-realloc" "-fno-builtin-free")
# static build
add_library(${PROJECT_NAME}_s STATIC ${CPP_SOURCES})
target_include_directories(${PROJECT_NAME}_s PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(${PROJECT_NAME}_s cpp_bloom)
target_compile_options(${PROJECT_NAME}_s PUBLIC "-march=native" "-Wall" "-fno-builtin-mallloc" "-fno-builtin-calloc" "-fno-builtin-realloc" "-fno-builtin-free")
#install()