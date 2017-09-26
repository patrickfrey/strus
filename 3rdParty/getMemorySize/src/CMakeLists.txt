cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
  getMemorySize.c
)

include_directories(
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/getMemorySize/include"
)

# -------------------------------------------
# SNOWBALL STEMMER LIBRARY
# -------------------------------------------
add_library( getmemsize_static STATIC ${source_files} )
set_property( TARGET getmemsize_static PROPERTY POSITION_INDEPENDENT_CODE TRUE )



