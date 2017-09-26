cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/queryproc/weighting"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( scalarFunction scalarFunction.cpp )
target_link_libraries( scalarFunction strus_scalarfunc strus_error strus_base strus_private_utils ${Boost_LIBRARIES} "${Intl_LIBRARIES}"  )

