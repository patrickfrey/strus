cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/utils"
)
link_directories(
	"${PROJECT_SOURCE_DIR}/src/utils" )

add_executable( testLoggingMalloc testLoggingMalloc.cpp )
target_link_libraries( testLoggingMalloc dl strus_utils strus_private_utils boost_system )

