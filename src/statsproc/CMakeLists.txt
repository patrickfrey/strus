cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	statisticsProcessor.cpp
	statisticsBuilder.cpp
	statisticsViewer.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/statsproc" 
	"${CNODETRIE_INCLUDE_DIRS}" 
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${MAIN_SOURCE_DIR}/utils"
	"${CNODETRIE_LIBRARY_DIRS}" 
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_statsproc SHARED ${source_files} libstrus_statsproc.cpp )
target_link_libraries( strus_statsproc strus_private_utils strus_base compactnodetrie_strus_static )

set_target_properties(
    strus_statsproc
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_statsproc
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

