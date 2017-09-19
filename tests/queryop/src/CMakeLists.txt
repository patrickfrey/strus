cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/storage"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/storage"
	"${MAIN_SOURCE_DIR}/queryeval"
	"${MAIN_SOURCE_DIR}/queryproc"
	"${MAIN_SOURCE_DIR}/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testQueryOp testQueryOp.cpp)
target_link_libraries( testQueryOp strus_error strus_storage strus_queryeval strus_queryproc strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

