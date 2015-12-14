cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/statsproc"
	"${PROJECT_SOURCE_DIR}/include" )

link_directories(
	"${PROJECT_SOURCE_DIR}/src/error"
	"${PROJECT_SOURCE_DIR}/src/statsproc"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testStatisticsProcessor testStatisticsProcessor.cpp)
target_link_libraries( testStatisticsProcessor strus_error strus_statsproc strus_utils boost_system )

