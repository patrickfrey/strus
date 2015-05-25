cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

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
add_library( getMemorySize_static STATIC ${source_files})



