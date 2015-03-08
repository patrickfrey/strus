cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/src/queryeval"
	"${PROJECT_SOURCE_DIR}/src/queryproc"
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testRandomCollection testRandomCollection.cpp)
target_link_libraries( testRandomCollection boost_system strus_storage strus_queryeval strus_queryproc strus_utils)

