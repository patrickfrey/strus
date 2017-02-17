# ----------------------------------------------------------------
# Defines a function to locate a strus related package
# ----------------------------------------------------------------
FUNCTION( find_strus_package pkgname )
   IF (${pkgname} STREQUAL "core")
   	set( pkgname "")
   ENDIF (${pkgname} STREQUAL "core")
   MESSAGE( STATUS  "Setting strus${pkgname} prefix path to ${CMAKE_INSTALL_PREFIX}")
   if(NOT strus${pkgname}_LIBRARY_DIRS)
     set( strus${pkgname}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
     set( strus${pkgname}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" PARENT_SCOPE )
   endif()
   if(NOT strus${pkgname}_INCLUDE_DIRS)
     set( strus${pkgname}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include" )
     set( strus${pkgname}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include" PARENT_SCOPE )
   endif()
   MESSAGE( STATUS  "Set strus${pkgname} include directories to ${strus${pkgname}_INCLUDE_DIRS}" )
   MESSAGE( STATUS  "Set strus${pkgname} linking directories to ${strus${pkgname}_LIBRARY_DIRS}" )
ENDFUNCTION( find_strus_package pkgname )

