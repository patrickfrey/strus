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
