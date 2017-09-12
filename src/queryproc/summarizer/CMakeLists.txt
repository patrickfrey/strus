cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	summarizer_standard.cpp
	summarizerAttribute.cpp
	summarizerForwardIndex.cpp
	summarizerListMatches.cpp
	summarizerMetaData.cpp
	summarizerMatchPhrase.cpp
	summarizerMatchVariables.cpp
	summarizerAccumulateVariable.cpp
	summarizerAccumulateNear.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${PROJECT_SOURCE_DIR}/src/queryproc/summarizer"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
  "${Boost_LIBRARY_DIRS}"
  "${CMAKE_CURRENT_BINARY_DIR}/../utils"
  "${CMAKE_CURRENT_BINARY_DIR}/../weighting"
  "${strusbase_LIBRARY_DIRS}"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( summarizer_standard STATIC ${source_files})
target_link_libraries( summarizer_standard strus_base queryproc_utils )
set_property( TARGET summarizer_standard PROPERTY POSITION_INDEPENDENT_CODE TRUE )


