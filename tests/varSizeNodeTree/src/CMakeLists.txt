cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/storage"
	"${STRUS_INCLUDE_DIRS}"
	"${CNODETRIE_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/storage"
	"${MAIN_SOURCE_DIR}/utils"
	"${CNODETRIE_LIBRARY_DIRS}"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testVarSizeNodeTree testVarSizeNodeTree.cpp)
target_link_libraries( testVarSizeNodeTree strus_base strus_private_utils strus_storage_static compactnodetrie_strus_static ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

