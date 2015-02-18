cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Project globals:
project(Strus)
set( STRUS_MAJOR_VERSION 0 )
set( STRUS_MINOR_VERSION 0 )
set( STRUS_PATCH_VERSION 1 )
set( STRUS_VERSION ${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}.${STRUS_PATCH_VERSION} )

if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_INSTALL_PREFIX /usr/local)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/strus")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

include(cmake/LinuxDistribution.cmake)

set_property(GLOBAL PROPERTY rule_launch_compile ccache)
set_property(GLOBAL PROPERTY rule_launch_link ccache)

if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "-std=c++98  -Wall -pedantic -g -Wfatal-errors -Werror -fvisibility=hidden -fPIC -O0")
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0504")
endif()

find_package( Boost 1.53.0 COMPONENTS thread system date_time )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )

enable_testing()

# Subdirectories:
add_subdirectory(src)
add_subdirectory(tests)
  
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


# Packaging (make package)
IF (UNIX)
project( strus )
set( CPACK_PACKAGE_VERSION "0.0.1" )
set( CPACK_PACKAGE_DIRECTORY /home/patrick/tmp )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library implementing the storage of a text search engine" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING" )
set( CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README" )
set( CPACK_RESOURCE_FILE_WELCOME "${CMAKE_CURRENT_SOURCE_DIR}/WELCOME" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README" )
set( CPACK_CMAKE_GENERATOR "Unix Makefiles" )
set( CPACK_INSTALL_CMAKE_PROJECTS "/home/patrick/Projects/github/strus;Strus;ALL;/" )
set( CPACK_SOURCE_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackSourceConfig.cmake" )
set( CPACK_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackConfig.cmake" )
set( CPACK_GENERATOR "RPM;DEB" )
set( CPACK_PACKAGE_NAME "strus" )
set( CPACK_PACKAGE_RELEASE 1 )
set( CPACK_PACKAGE_LICENSE "GPL v3" )
set( CPACK_RPM_PACKAGE_LICENSE "GPL v3" )
set( CPACK_PACKAGE_CONTACT "PatrickFrey@project-strus.net" )
set( CPACK_PACKAGE_VENDOR "project-strus.net" )
set( CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
set( CPACK_RPM_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}" )
set( CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG" )
set( CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}" )
include( CPack )
ENDIF (UNIX)
