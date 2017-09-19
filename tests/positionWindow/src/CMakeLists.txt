cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/queryproc/utils"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/queryproc/utils"
	"${MAIN_SOURCE_DIR}/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testPositionWindow testPositionWindow.cpp)
target_link_libraries( testPositionWindow strus_error queryproc_utils ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

