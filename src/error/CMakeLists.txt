cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	errorBuffer.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/error" 
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_error SHARED ${source_files} libstrus_error.cpp )
target_link_libraries( strus_error strus_private_utils )

set_target_properties(
    strus_error
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_error
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

