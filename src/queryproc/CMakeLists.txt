cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory( utils )
add_subdirectory( summarizer )
add_subdirectory( iterator )
add_subdirectory( weighting )

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	weightingFunctionSummarizer.cpp
	queryProcessor.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/queryproc"
	"${strusbase_INCLUDE_DIRS}"
)

link_directories(
	${Boost_LIBRARY_DIRS}
	"${CMAKE_CURRENT_BINARY_DIR}/utils"
	"${CMAKE_CURRENT_BINARY_DIR}/iterator"
	"${CMAKE_CURRENT_BINARY_DIR}/summarizer"
	"${CMAKE_CURRENT_BINARY_DIR}/weighting"
	"${strusbase_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_cppcheck( strus_queryproc ${source_files} libstrus_queryproc.cpp )

add_library( strus_queryproc SHARED ${source_files} libstrus_queryproc.cpp )
target_link_libraries( strus_queryproc strus_private_utils strus_base queryproc_utils iterator_standard summarizer_standard weighting_standard strus_scalarfunc )

set_target_properties(
    strus_queryproc
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_queryproc
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

