cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	summarizer_standard.cpp
	summarizerAttribute.cpp
	summarizerContent.cpp
	summarizerMetaData.cpp
	summarizerStructureHeader.cpp
	summarizerMatchPhrase.cpp
	summarizerMatchesBase.cpp
	summarizerListMatches.cpp
	summarizerAccumulateMatches.cpp
	summarizerAccumulateNear.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${Intl_INCLUDE_DIRS}"
  "${STRUS_INCLUDE_DIRS}"
  "${MAIN_SOURCE_DIR}/queryproc/utils"
  "${MAIN_SOURCE_DIR}/queryproc/summarizer"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
  ${Boost_LIBRARY_DIRS}
  "${MAIN_SOURCE_DIR}/utils"
  "${MAIN_SOURCE_DIR}/weighting"
  "${strusbase_LIBRARY_DIRS}"
)

# ------------------------------------
# LIBRARY
# ------------------------------------
add_cppcheck( summarizer_standard ${source_files} )

add_library( summarizer_standard STATIC ${source_files})
target_link_libraries( summarizer_standard strus_base queryproc_utils )
set_property( TARGET summarizer_standard PROPERTY POSITION_INDEPENDENT_CODE TRUE )


