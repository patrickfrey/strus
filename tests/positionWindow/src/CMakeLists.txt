cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/queryproc/utils"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/queryproc/utils"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testPositionWindow testPositionWindow.cpp)
target_link_libraries( testPositionWindow strus_error queryproc_utils ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

