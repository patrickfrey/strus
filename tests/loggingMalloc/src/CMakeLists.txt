cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/utils"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testLoggingMalloc testLoggingMalloc.cpp )
target_link_libraries( testLoggingMalloc ${CMAKE_DL_LIBS} strus_base strus_private_utils ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

