cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	positionWindow.cpp
	proximityWeightAccumulator.cpp
	postingIteratorHelpers.cpp
	sentenceIterator.cpp
)

include_directories(
  "${Boost_INCLUDE_DIRS}"
  "${Intl_INCLUDE_DIRS}"
  "${STRUS_INCLUDE_DIRS}"
  "${MAIN_SOURCE_DIR}/queryproc/utils"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${strusbase_LIBRARY_DIRS}"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_cppcheck( queryproc_utils ${source_files} )

add_library( queryproc_utils STATIC ${source_files})
target_link_libraries( queryproc_utils strus_private_utils strus_base )
set_property( TARGET queryproc_utils PROPERTY POSITION_INDEPENDENT_CODE TRUE )



