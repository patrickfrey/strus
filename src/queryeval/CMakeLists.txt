cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	accumulator.cpp
	queryEval.cpp
	query.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	${Boost_LIBRARY_DIRS}
	"${MAIN_SOURCE_DIR}/utils"
	"${strusbase_LIBRARY_DIRS}"
)


# --------------------------------
# QUERY EVAL LIBRARY
# Build a shared library as deployment artefact and a static library for tests that need to bypass the library interface to do their job
# --------------------------------
add_cppcheck( strus_queryeval_static ${source_files} libstrus_queryeval.cpp )

add_library( strus_queryeval_static STATIC ${source_files})
target_link_libraries( strus_queryeval_static strus_base strus_private_utils )
set_property( TARGET strus_queryeval_static PROPERTY POSITION_INDEPENDENT_CODE TRUE )

add_library( strus_queryeval SHARED libstrus_queryeval.cpp )
target_link_libraries( strus_queryeval strus_queryeval_static  strus_base strus_private_utils "${Boost_LIBRARIES}" )

set_target_properties(
    strus_queryeval
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_queryeval
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

