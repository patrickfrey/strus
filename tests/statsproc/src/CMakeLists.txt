cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_TESTS_DIR}/utils"
	"${MAIN_SOURCE_DIR}/statsproc"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${MAIN_SOURCE_DIR}/utils"
	"${MAIN_SOURCE_DIR}/statsproc"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
)

add_executable( testStatisticsProcessor testStatisticsProcessor.cpp)
target_link_libraries( testStatisticsProcessor strus_error strus_statsproc strus_base strus_filelocator ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

