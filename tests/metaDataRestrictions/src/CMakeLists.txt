cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/src/queryeval"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testMetaDataRestrictions  testMetaDataRestrictions.cpp)
target_link_libraries( testMetaDataRestrictions strus_queryeval_static strus_error strus_base strus_storage_static ${Boost_LIBRARIES} ${Intl_LIBRARIES})

