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
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/summarizer"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_library( summarizer_standard STATIC ${source_files})
target_link_libraries( summarizer_standard strus_utils )


