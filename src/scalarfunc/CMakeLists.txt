cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	scalarFunction.cpp
	scalarFunctionInstance.cpp
	scalarFunctionParser.cpp
	scalarFunctionLinearComb.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/src/scalarfunc" 
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_library( strus_scalarfunc SHARED ${source_files} libstrus_scalarfunc.cpp )
target_link_libraries( strus_scalarfunc strus_base strus_private_utils )

set_target_properties(
    strus_scalarfunc
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_scalarfunc
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

