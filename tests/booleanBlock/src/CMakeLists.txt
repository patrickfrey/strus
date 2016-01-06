cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testBooleanBlock testBooleanBlock.cpp)
target_link_libraries( testBooleanBlock strus_utils strus_storage_static ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

