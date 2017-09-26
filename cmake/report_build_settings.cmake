# ---------------------------------------------------------------------------
# Defines what cmake should report when configuring the build
# ---------------------------------------------------------------------------

# Message:
MESSAGE("Platform:")
MESSAGE("  Host: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
MESSAGE("  Canonical: ${INSTALLER_PLATFORM}")
MESSAGE("  CMake: ${CMAKE_VERSION}")
MESSAGE("  CMake generator: ${CMAKE_GENERATOR}")
MESSAGE("  CMake build tool: ${CMAKE_BUILD_TOOL}")
MESSAGE("  CMake build type: ${CMAKE_BUILD_TYPE}")

MESSAGE("Compiler:")
MESSAGE("  C/C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
if( CMAKE_BUILD_TYPE STREQUAL "Debug")
MESSAGE("  C++ compilation flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE("  C compilation flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
endif()
if( CMAKE_BUILD_TYPE STREQUAL "Release")
MESSAGE("  C++ compilation flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE("  C compilation flags: ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
endif()
if( CMAKE_BUILD_TYPE STREQUAL "")
MESSAGE("  C++ compilation flags: ${CMAKE_CXX_FLAGS}")
MESSAGE("  C compilation flags: ${CMAKE_C_FLAGS}")
endif()
