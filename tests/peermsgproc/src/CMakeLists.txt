cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/peermsgproc"
	"${PROJECT_SOURCE_DIR}/include" )

link_directories(
	"${PROJECT_SOURCE_DIR}/src/error"
	"${PROJECT_SOURCE_DIR}/src/peermsgproc"
	"${Boost_LIBRARY_DIRS}" )

add_executable( testPeerMessageProcessor testPeerMessageProcessor.cpp)
target_link_libraries( testPeerMessageProcessor boost_system strus_error strus_peermsgproc strus_utils )

