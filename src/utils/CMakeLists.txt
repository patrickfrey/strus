cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	stringMap.cpp
	internationalization.cpp
	cstring.c
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${MAIN_SOURCE_DIR}/utils"
	"${strusbase_LIBRARY_DIRS}"
)


# -------------------------------------------
# UTILS LIBRARY
# Library with some helper functions used by strus libraries and programs but not part of the public interface
# -------------------------------------------
add_library( strus_private_utils STATIC ${source_files} )
target_link_libraries( strus_private_utils ${Intl_LIBRARIES} ${Boost_LIBRARIES} )
set_property( TARGET strus_private_utils PROPERTY POSITION_INDEPENDENT_CODE TRUE )

add_library( strus_malloc_unreliable SHARED unreliableMalloc.c )
target_link_libraries( strus_malloc_unreliable ${CMAKE_DL_LIBS} )
set_target_properties(
    strus_malloc_unreliable
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

add_library( strus_malloc_logging SHARED loggingMalloc.c )
target_link_libraries( strus_malloc_logging ${CMAKE_DL_LIBS} strus_base )
set_target_properties(
    strus_malloc_logging
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_malloc_unreliable
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

install( TARGETS strus_malloc_logging
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

