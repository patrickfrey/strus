cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	positionWindow.cpp
	proximityWeightAccumulator.cpp
	postingIteratorHelpers.cpp
	structureIterator.cpp
)

include_directories(
  "${Boost_INCLUDE_DIRS}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${strusbase_LIBRARY_DIRS}"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( queryproc_utils STATIC ${source_files})
target_link_libraries( queryproc_utils strus_private_utils strus_base )
set_property( TARGET queryproc_utils PROPERTY POSITION_INDEPENDENT_CODE TRUE )



