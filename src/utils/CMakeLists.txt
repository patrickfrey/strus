cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_shared_utils_source_files
	cmdLineOpt.cpp
	configParser.cpp
	fileio.cpp
	snprintf.c
)

set( libstrus_private_utils_source_files
	utils.cpp
	internationalization.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils" )


# -------------------------------------------
# UTILS LIBRARY
# Library with some helper functions used by strus libraries and programs but not part of the public interface
# -------------------------------------------
add_library( strus_private_utils STATIC ${libstrus_private_utils_source_files} )
target_link_libraries( strus_private_utils  "${Boost_LIBRARIES}" )
add_library( strus_utils SHARED ${libstrus_shared_utils_source_files} )
target_link_libraries( strus_utils  "${Boost_LIBRARIES}"  strus_private_utils )
set_target_properties(
    strus_utils
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

add_library( strus_unreliable_malloc SHARED unreliableMalloc.c )
target_link_libraries( strus_unreliable_malloc  dl )
set_target_properties(
    strus_unreliable_malloc
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_utils
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

install( TARGETS strus_unreliable_malloc
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )


