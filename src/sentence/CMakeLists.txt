cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	sentenceAnalyzerInstance.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${MAIN_SOURCE_DIR}/sentence" 
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
add_cppcheck( strus_sentence  ${source_files} libstrus_sentence.cpp )

add_library( strus_sentence SHARED ${source_files} libstrus_sentence.cpp )
target_link_libraries( strus_sentence strus_base strus_private_utils )

set_target_properties(
    strus_sentence
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_sentence
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

