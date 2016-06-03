cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory( utils )
add_subdirectory( summarizer )
add_subdirectory( iterator )
add_subdirectory( weighting )

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	queryProcessor.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/queryproc"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${PROJECT_SOURCE_DIR}/src/queryproc/iterator"
	"${PROJECT_SOURCE_DIR}/src/queryproc/summarizer"
	"${PROJECT_SOURCE_DIR}/src/queryproc/weighting"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_queryproc SHARED ${source_files} libstrus_queryproc.cpp )
target_link_libraries( strus_queryproc strus_private_utils strus_base queryproc_utils iterator_standard summarizer_standard weighting_standard strus_scalarfunc )

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
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

