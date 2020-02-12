cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/storage"
	"${STRUS_INCLUDE_DIRS}"
	"${QPROC_UTILS_INCLUDE_DIR}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${MAIN_SOURCE_DIR}/storage"
	"${MAIN_SOURCE_DIR}/utils"
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
	"${QPROC_UTILS_LIBRARY_DIR}"
)

add_executable( functions functions.cpp)
target_link_libraries( functions strus_base strus_storage_static queryproc_utils ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

