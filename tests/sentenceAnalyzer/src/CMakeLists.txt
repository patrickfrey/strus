cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/storage"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/storage"
	"${MAIN_SOURCE_DIR}/utils"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
)

add_executable( testSentenceAnalyzer testSentenceAnalyzer.cpp)
target_link_libraries( testSentenceAnalyzer strus_base strus_error strus_private_utils strus_sentence ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

