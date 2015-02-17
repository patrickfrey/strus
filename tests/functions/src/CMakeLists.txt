cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories( ${Boost_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/src/storage" "${PROJECT_SOURCE_DIR}/include" )
link_directories( /usr/local/lib)

add_executable( functions functions.cpp)

