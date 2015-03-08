cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory( summarizer )
add_subdirectory( iterator )
add_subdirectory( weighting )

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	queryProcessor.cpp
	libstrus_queryproc.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/queryproc" )

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
	"${PROJECT_SOURCE_DIR}/src/queryproc/iterator"
	"${PROJECT_SOURCE_DIR}/src/queryproc/summarize"
	"${PROJECT_SOURCE_DIR}/src/queryproc/weighting" )

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_queryproc SHARED ${source_files})
target_link_libraries( strus_queryproc strus_private_utils strus_utils iterator_standard summarizer_standard weighting_standard)
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

