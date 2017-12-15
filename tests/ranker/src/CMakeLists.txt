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
	"${MAIN_SOURCE_DIR}/storage"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testRanker testRanker.cpp)
target_link_libraries( testRanker strus_base strus_queryeval_static ${Boost_LIBRARIES} ${Intl_LIBRARIES})

