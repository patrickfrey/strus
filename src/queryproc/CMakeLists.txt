cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_queryproc_source_files
libstrus_queryproc.cpp
queryProcessor.cpp
libstrus_queryproc.cpp
weighting/weightingBM25.cpp
iterator/postingIteratorIntersect.cpp
iterator/postingIteratorStructWithin.cpp
iterator/postingIteratorHelpers.cpp
iterator/postingIteratorContains.cpp
iterator/postingIteratorUnionWeighted.cpp
iterator/postingIteratorUnion.cpp
iterator/postingIteratorStructSequence.cpp
iterator/postingIteratorDifference.cpp
summarizer/summarizerAttribute.cpp
summarizer/summarizerListMatches.cpp
summarizer/summarizerMetaData.cpp
summarizer/summarizerMatchPhrase.cpp
queryProcessor.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc"
)

# -------------------------------------------
# QUERY PROCESSOR LIBRARY
# -------------------------------------------
add_library(strus_queryproc SHARED ${libstrus_queryproc_source_files})
target_link_libraries( strus_queryproc strus_utils )
set_target_properties(
    strus_queryproc
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_queryproc
           LIBRARY DESTINATION lib/strus )

