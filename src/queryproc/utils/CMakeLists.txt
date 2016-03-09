cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	positionWindow.cpp
	proximityWeightAccumulator.cpp
)

include_directories(
  "${Boost_INCLUDE_DIRS}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( queryproc_utils STATIC ${source_files})
target_link_libraries( queryproc_utils strus_private_utils strus_utils )



