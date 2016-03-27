cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${Boost_LIBRARY_DIRS}" )

add_executable( scalarFunction scalarFunction.cpp )
target_link_libraries( scalarFunction strus_scalarfunc strus_error strus_utils strus_private_utils ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

