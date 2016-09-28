cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/3rdParty/armadillo/include"
	"${PROJECT_SOURCE_DIR}/src/vectorspace"
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${PROJECT_SOURCE_DIR}/src/vectorspace"
	"${PROJECT_SOURCE_DIR}/3rdParty/armadillo"
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_executable( testArmadillo testArmadillo.cpp)
target_link_libraries( testArmadillo boost_system strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} strus_armadillo )

add_executable( testLshSimHash testLshSimHash.cpp)
target_link_libraries( testLshSimHash strus_vectorspace_static  ${Boost_LIBRARIES} ${Intl_LIBRARIES} strus_armadillo )

add_executable( testLshVectorSpaceModel testLshVectorSpaceModel.cpp)
target_link_libraries( testLshVectorSpaceModel boost_system strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} strus_armadillo )

add_executable( testVectorSpaceModel testVectorSpaceModel.cpp)
target_link_libraries( testVectorSpaceModel boost_system strus_base strus_error strus_vectorspace ${Boost_LIBRARIES} ${Intl_LIBRARIES} strus_armadillo )


