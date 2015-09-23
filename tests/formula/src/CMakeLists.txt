cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${Boost_LIBRARY_DIRS}" )

add_executable( formula formula.cpp "${PROJECT_SOURCE_DIR}/src/queryproc/weighting/formulaInterpreter.cpp" )
target_link_libraries( formula strus_utils boost_system )

