cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_utils_source_files
   arithmeticVariantAsString.cpp
   cmdLineOpt.cpp
   configParser.cpp
   fileio.cpp
   protocol.cpp
   utils.cpp  
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
add_library( strus_private_utils STATIC utils.cpp)
target_link_libraries( strus_private_utils  "${Boost_LIBRARIES}" )
add_library( strus_utils SHARED ${libstrus_utils_source_files})
target_link_libraries( strus_utils  "${Boost_LIBRARIES}" )
set_target_properties(
    strus_utils
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


