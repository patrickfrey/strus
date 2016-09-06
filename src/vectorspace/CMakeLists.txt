cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(
	libstrus_vectorspace_source_files
	random.cpp    
	simhash.cpp
	lshmodel.cpp
	genmodel.cpp 
	vectorSpaceModel.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	${ARMADILLO_INCLUDE_DIRS}
	"${PROJECT_SOURCE_DIR}/include"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/src/utils"
	"${PROJECT_SOURCE_DIR}/src/vectorspace"
	"${strusbase_LIBRARY_DIRS}"
)

# ----------------------------------------------
# VECTOR SPACE MODEL LIBRARY
# ----------------------------------------------
add_library( strus_vectorspace_static STATIC ${libstrus_vectorspace_source_files} )
target_link_libraries( strus_vectorspace_static strus_base strus_private_utils ${ARMADILLO_LIBRARIES} )

add_library( strus_vectorspace SHARED libstrus_vectorspace.cpp )
target_link_libraries( strus_vectorspace strus_vectorspace_static strus_base strus_private_utils ${ARMADILLO_LIBRARIES} )
set_target_properties(
    strus_vectorspace
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_vectorspace
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

