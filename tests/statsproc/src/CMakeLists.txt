cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/tests/utils"
	"${PROJECT_SOURCE_DIR}/src/statsproc"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${PROJECT_SOURCE_DIR}/src/statsproc"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testStatisticsProcessor testStatisticsProcessor.cpp)
target_link_libraries( testStatisticsProcessor strus_error strus_statsproc strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

