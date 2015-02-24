cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	weighting_standard.cpp
	weightingBM25.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/weighting_bm25"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( weighting_standard STATIC ${source_files})
target_link_libraries( weighting_standard strus_utils )



