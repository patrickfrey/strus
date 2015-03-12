cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/src/queryeval"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testMetaDataRestrictions  testMetaDataRestrictions.cpp)
target_link_libraries( testMetaDataRestrictions boost_system strus_queryeval_static strus_utils strus_storage_static )

