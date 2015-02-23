cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories( ${Boost_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/src/storage" "${PROJECT_SOURCE_DIR}/include" )
link_directories( /usr/local/lib "${PROJECT_SOURCE_DIR}/src/storage")

add_executable( testBooleanBlock testBooleanBlock.cpp)
target_link_libraries( testBooleanBlock boost_system strus_utils strus_storage_static)

