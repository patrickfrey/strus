cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/storage"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/queryeval"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/queryproc"
	"${CMAKE_CURRENT_BINARY_DIR}/../../../src/utils"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testQueryOp testQueryOp.cpp)
target_link_libraries( testQueryOp strus_error strus_storage strus_queryeval strus_queryproc strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

