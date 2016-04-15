cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# must be done BEFORE project in order to work!
include(cmake/install_prefix.cmake)

# Project globals:
project( Strus )
set( STRUS_MAJOR_VERSION 0 )
set( STRUS_MINOR_VERSION 8 )
set( STRUS_PATCH_VERSION 2 )
set( STRUS_VERSION ${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}.${STRUS_PATCH_VERSION} )

include( cmake/dist.cmake )
include( cmake/LinuxDistribution.cmake )
include( cmake/build_rules.cmake )
include( cmake/boost.cmake )
include( cmake/intl.cmake )
include( cmake/find_strus_package.cmake )

find_strus_package( base )
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
configure_file( "${PROJECT_SOURCE_DIR}/include/strus/versionStorage.hpp.in"  "${PROJECT_SOURCE_DIR}/include/strus/versionStorage.hpp"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh.in"  "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/strus.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/strus.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/strus-Debian_7.0.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/strus-Debian_7.0.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/strus-xUbuntu_12.04.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/strus-xUbuntu_12.04.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strus.spec.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strus.spec"  @ONLY )

set( STRUS_GETTEXT_PACKAGE "strus-dom" )	#... 1st parameter of bindtextdomain(...)
set( STRUS_GETTEXT_LOCALEDIR "" )			#... 2nd parameter of bindtextdomain(...)
configure_file( "${PROJECT_SOURCE_DIR}/src/utils/internationalization.cpp.in"  "${PROJECT_SOURCE_DIR}/src/utils/internationalization.cpp"  @ONLY )

enable_testing()

# Subdirectories:
add_subdirectory( 3rdParty )
add_subdirectory( src )
add_subdirectory( tests )
  
include( cmake/report_build_settings.cmake )

# Install header files:
install(DIRECTORY include/strus
          DESTINATION include
          FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY include/strus
          DESTINATION include
          FILES_MATCHING PATTERN "*.h")

include( cmake/standard_targets.cmake )
