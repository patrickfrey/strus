cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	summarizer_standard.cpp
	summarizerAttribute.cpp
	summarizerListMatches.cpp
	summarizerMetaData.cpp
	summarizerMatchPhrase.cpp
	summarizerMatchVariables.cpp
	summarizerAccumulateVariable.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${PROJECT_SOURCE_DIR}/src/queryproc/summarizer"
)

link_directories(
  "${Boost_LIBRARY_DIRS}"
  "${PROJECT_SOURCE_DIR}/src/queryproc/utils"
  "${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( summarizer_standard STATIC ${source_files})
target_link_libraries( summarizer_standard strus_utils queryproc_utils )


