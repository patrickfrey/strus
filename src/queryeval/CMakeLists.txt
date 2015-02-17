cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_queryeval_source_files
summarizerConfig.cpp
weightingConfig.cpp
accumulator.cpp
metaDataRestriction.cpp
weightingDef.cpp
queryEval.cpp
query.cpp
summarizerDef.cpp
libstrus_queryeval.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
)


# --------------------------------
# QUERY EVAL LIBRARY
# Build a shared library as deployment artefact and a static library for tests that need to bypass the library interface to do their job
# --------------------------------
add_library( strus_queryeval SHARED ${libstrus_queryeval_source_files})
add_library( strus_queryeval_static STATIC ${libstrus_queryeval_source_files})


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_queryeval
           LIBRARY DESTINATION lib/strus )

