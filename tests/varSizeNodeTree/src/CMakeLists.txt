cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/src/utils" )

add_executable( testVarSizeNodeTree testVarSizeNodeTree.cpp)
target_link_libraries( testVarSizeNodeTree boost_system strus_storage_static strus_utils strus_private_utils)

