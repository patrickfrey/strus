cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	weighting_standard.cpp
	weightingSmart.cpp
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
)

link_directories(
  "${Boost_LIBRARY_DIRS}"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( weighting_standard STATIC ${source_files})
target_link_libraries( weighting_standard strus_private_utils strus_utils queryproc_utils )



