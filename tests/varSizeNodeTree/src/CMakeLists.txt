cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/src" )

add_executable( testVarSizeNodeTree testVarSizeNodeTree.cpp)
target_link_libraries( testVarSizeNodeTree boost_system strus_utils strus_private_utils strus_storage_static compactnodetrie_static )

