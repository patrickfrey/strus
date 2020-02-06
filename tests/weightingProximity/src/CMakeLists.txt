cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_TESTS_DIR}/utils"
	"${strusbase_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
)

link_directories(
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
)

add_executable( testWeightingProximity testWeightingProximity.cpp)
target_link_libraries( testWeightingProximity strus_error strus_filelocator strus_storage strus_queryeval strus_queryproc strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES})

