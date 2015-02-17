cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_utils_source_files
   arithmeticVariantAsString.cpp
   cmdLineOpt.cpp
   configParser.cpp
   fileio.cpp
   protocol.cpp  
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
)


# -------------------------------------------
# UTILS LIBRARY
# Library with some helper functions used by strus libraries and programs but not part of the public interface
# -------------------------------------------
add_library(strus_utils SHARED ${libstrus_utils_source_files})


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_utils
           LIBRARY DESTINATION lib/strus )


