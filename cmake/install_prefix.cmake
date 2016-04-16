# -------------------------------------------------------------------------------
# Defines the prefix for the installation path and link policies
# NOTE: This file has to be inlcuded before defining anything else
# -------------------------------------------------------------------------------

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(NOT WIN32)
    set( CMAKE_INSTALL_PREFIX /usr/local )
    if(NOT LIB_INSTALL_DIR)
      set( LIB_INSTALL_DIR lib )
    endif(NOT LIB_INSTALL_DIR)
  endif(NOT WIN32)
else(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(NOT LIB_INSTALL_DIR)
      set( LIB_INSTALL_DIR lib )
    endif(NOT LIB_INSTALL_DIR)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
if(NOT APPLE)
    set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE ) 
    set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
    set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
    set( CMAKE_NO_BUILTIN_CHRPATH TRUE )
endif(NOT APPLE)
