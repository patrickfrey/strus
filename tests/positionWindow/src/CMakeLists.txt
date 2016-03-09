cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/queryproc/utils"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/queryproc/utils"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testPositionWindow testPositionWindow.cpp)
target_link_libraries( testPositionWindow strus_error queryproc_utils ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

