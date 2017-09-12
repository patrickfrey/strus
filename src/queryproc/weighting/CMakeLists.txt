cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	weighting_standard.cpp
	weightingSmart.cpp
	weightingScalar.cpp
	weightingBM25.cpp
	weightingBM25pff.cpp
	weightingFrequency.cpp
	weightingConstant.cpp
	weightingMetadata.cpp
)

include_directories(
  "${Boost_INCLUDE_DIRS}"
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
  "${Boost_LIBRARY_DIRS}"
  "${CMAKE_CURRENT_BINARY_DIR}/../utils"
  "${strusbase_LIBRARY_DIRS}"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( weighting_standard STATIC ${source_files})
target_link_libraries( weighting_standard strus_private_utils strus_base queryproc_utils )
set_property( TARGET weighting_standard PROPERTY POSITION_INDEPENDENT_CODE TRUE )



