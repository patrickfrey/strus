cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	${ARMADILLO_INCLUDE_DIRS}
	"${PROJECT_SOURCE_DIR}/src/vectorspace"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${PROJECT_SOURCE_DIR}/src/storage"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testArmadillo testArmadillo.cpp)
target_link_libraries( testArmadillo boost_system strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} ${ARMADILLO_LIBRARIES} )

