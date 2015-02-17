cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Project globals:
project(Strus)
set(STRUS_MAJOR_VERSION 0)
set(STRUS_MINOR_VERSION 0)
set(STRUS_PATCH_VERSION 1)
set(STRUS_VERSION ${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}.${STRUS_PATCH_VERSION})

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

# Packaging
CPACK_PACKAGE_NAME strus
CPACK_PACKAGE_VERSION 0.0.1
CPACK_PACKAGE_DIRECTORY pkg
CPACK_PACKAGE_VENDOR strus

