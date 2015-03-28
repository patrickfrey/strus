cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# must be done BEFORE project in order to work!
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(NOT WIN32)
    set( CMAKE_INSTALL_PREFIX /usr/local )
  endif(NOT WIN32)
else(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE ) 
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )

# Project globals:
project( Strus )
set( STRUS_MAJOR_VERSION 0 )
set( STRUS_MINOR_VERSION 0 )
set( STRUS_PATCH_VERSION 1 )
set( STRUS_VERSION ${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}.${STRUS_PATCH_VERSION} )

include(cmake/dist.cmake)
include(cmake/LinuxDistribution.cmake)

if(CMAKE_COMPILER_IS_GNUCXX)
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/strus" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
endif()

set_property(GLOBAL PROPERTY rule_launch_compile ccache)
set_property(GLOBAL PROPERTY rule_launch_link ccache)

if(CMAKE_COMPILER_IS_GNUCXX)
set( STRUS_OPTIMIZATION_LEVEL "0" )
set( CMAKE_CXX_FLAGS "-std=c++98  -Wall -pedantic -g -Wfatal-errors -Werror -fvisibility=hidden -fPIC -O${STRUS_OPTIMIZATION_LEVEL}" )
set( CMAKE_C_FLAGS "-std=c99 -Wall -pedantic -Wfatal-errors -Werror -fPIC -O${STRUS_OPTIMIZATION_LEVEL}" )
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0504")
endif()

find_package( Boost 1.53.0 COMPONENTS thread system date_time )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
configure_file( "${PROJECT_SOURCE_DIR}/include/strus/versionStorage.hpp.in"  "${PROJECT_SOURCE_DIR}/include/strus/versionStorage.hpp"  @ONLY )

set( STRUS_GETTEXT_PACKAGE "strus-dom" )	#... 1st parameter of bindtextdomain(...)
set( STRUS_GETTEXT_LOCALEDIR "" )			#... 2nd parameter of bindtextdomain(...)
configure_file( "${PROJECT_SOURCE_DIR}/src/utils/internationalization.cpp.in"  "${PROJECT_SOURCE_DIR}/src/utils/internationalization.cpp"  @ONLY )

IF( WIN32 )
set( STRUS_FILEIO_DIRSEP  '\\' )
ELSE( WIN32 )
set( STRUS_FILEIO_DIRSEP  '/' )
ENDIF( WIN32 )
configure_file( "${PROJECT_SOURCE_DIR}/src/utils/fileio_sys.hpp.in"  "${PROJECT_SOURCE_DIR}/src/utils/fileio_sys.hpp"  @ONLY )

enable_testing()

# Subdirectories:
add_subdirectory( 3rdParty )
add_subdirectory( src )
add_subdirectory( tests )
  
# Message:
MESSAGE("Platform:")
MESSAGE("  Host: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
MESSAGE("  Canonical: ${INSTALLER_PLATFORM}")
MESSAGE("  CMake: ${CMAKE_VERSION}")
MESSAGE("  CMake generator: ${CMAKE_GENERATOR}")
MESSAGE("  CMake build tool: ${CMAKE_BUILD_TOOL}")

MESSAGE("Compiler:")
MESSAGE("  C++ compilation flags: ${CMAKE_CXX_FLAGS}")
MESSAGE("  C compilation flags: ${CMAKE_C_FLAGS}")

# Install header files:
install(DIRECTORY include/strus
          DESTINATION include
          FILES_MATCHING PATTERN "*.hpp")

# Uninstall target:
configure_file(
    "${CMAKE_MODULE_PATH}/uninstall.cmake.in"
    "${CMAKE_MODULE_PATH}/uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_MODULE_PATH}/uninstall.cmake")


# Make distclean:
IF (UNIX)
  ADD_CUSTOM_TARGET (distclean @echo cleaning for source distribution)
  SET(DISTCLEANED
   cmake.depends
   cmake.check_depends
   CMakeCache.txt
   cmake.check_cache
   *.cmake
   Makefile
   core core.*
   *~
  )
  
  ADD_CUSTOM_COMMAND(
    DEPENDS clean
    COMMENT "distribution clean"
    COMMAND rm
    ARGS    -Rf pkg ${DISTCLEANED}
    TARGET  distclean
  )
ENDIF(UNIX)


# Packaging (make package)
IF (UNIX)
set( CPACK_PACKAGE_VERSION "0.0" )
set( CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/pkg" )
set( CPACK_PACKAGE_DESCRIPTION "Library implementing the storage of a text search engine" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library implementing the storage of a text search engine" )
set( CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING" )
set( CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README" )
set( CPACK_RESOURCE_FILE_WELCOME "${PROJECT_SOURCE_DIR}/WELCOME" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README" )
set( CPACK_CMAKE_GENERATOR "Unix Makefiles" )
set( CPACK_INSTALL_CMAKE_PROJECTS "${PROJECT_SOURCE_DIR};${PROJECT_NAME};ALL;/" )
set( CPACK_GENERATOR "RPM;DEB" )
string( TOLOWER "${PROJECT_NAME}" CPACK_PACKAGE_NAME)
set( CPACK_PACKAGE_RELEASE 1 )
set( CPACK_PACKAGE_LICENSE "GPL3" )
set( CPACK_PACKAGE_GROUP "Development/Libraries/C++" )
set( CPACK_PACKAGE_CONTACT "Patrick Frey <patrickfrey@project-strus.net>" )
set( CPACK_PACKAGE_VENDOR "project-strus.net" )
set( CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}" )
set( CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
set( CPACK_RPM_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}" )
set( CPACK_RPM_CHANGELOG_FILE "${PROJECT_SOURCE_DIR}/CHANGELOG" )
set( CPACK_RPM_PACKAGE_LICENSE "GPL3" )
set( CPACK_RPM_PACKAGE_GROUP "Development/Libraries/C++" )
set( CPACK_RPM_PACKAGE_DEPENDS  "cmake >= 2.8" )
set( CPACK_RPM_PACKAGE_REQUIRES  "libboost_system >= 1.53, libleveldb >= 1, libsnappy >= 1, libstdc++ >= 6, libc >= 6, libm >= 6, libpthread >= 0" )
set( CPACK_RPM_PACKAGE_URL "project-strus.net" )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "Patrick Frey <patrickfrey@project-strus.net>" )
set( CPACK_DEBIAN_PACKAGE_LICENSE "GPL3" )
include(CPack)
ENDIF (UNIX)
