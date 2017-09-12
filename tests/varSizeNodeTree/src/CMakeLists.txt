cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/storage"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/utils"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../3rdParty/compactNodeTrie/src"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testVarSizeNodeTree testVarSizeNodeTree.cpp)
target_link_libraries( testVarSizeNodeTree strus_base strus_private_utils strus_storage_static compactnodetrie_strus_static ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

