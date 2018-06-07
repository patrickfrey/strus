cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	programLoader.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/prgload_std" 
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	${Boost_LIBRARY_DIRS}
	"${MAIN_SOURCE_DIR}/utils"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_cppcheck( strus_storage_prgload_std ${source_files} libstrus_storage_prgload_std.cpp )

add_library( strus_storage_prgload_std SHARED ${source_files} libstrus_storage_prgload_std.cpp )
target_link_libraries( strus_storage_prgload_std strus_base strus_private_utils )

set_target_properties(
    strus_storage_prgload_std
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_storage_prgload_std
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

