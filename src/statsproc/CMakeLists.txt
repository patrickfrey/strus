cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

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
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/statsproc" 
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include" 
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/src" 
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_statsproc SHARED ${source_files} libstrus_statsproc.cpp )
target_link_libraries( strus_statsproc strus_private_utils strus_utils compactnodetrie_static )

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

