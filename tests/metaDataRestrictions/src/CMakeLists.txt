cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/storage"
	"${MAIN_SOURCE_DIR}/queryeval"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/utils"
	"${MAIN_SOURCE_DIR}/storage"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testMetaDataRestrictions  testMetaDataRestrictions.cpp)
target_link_libraries( testMetaDataRestrictions strus_queryeval_static strus_error strus_base strus_storage_static ${Boost_LIBRARIES} ${Intl_LIBRARIES})

