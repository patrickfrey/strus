# ------------------------------------------------------------------------------
# Defines some standard targets not directly supported by cmake
# ------------------------------------------------------------------------------

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
   *~ */*~ */*/*~ */*/*/*~
   CMakeFiles */CMakeFiles */*/CMakeFiles */*/*/CMakeFiles
   cmake_install.cmake */cmake_install.cmake */*/cmake_install.cmake */*/*/cmake_install.cmake
   Makefile */Makefile */*/Makefile */*/*/Makefile
   cmake/uninstall.cmake
   install_manifest.txt
   Testing
   CTestTestfile.cmake */CTestTestfile.cmake */*/CTestTestfile.cmake */*/*/CTestTestfile.cmake
  )

  ADD_CUSTOM_COMMAND(
    COMMAND ${CMAKE_BUILD_TOOL}
    ARGS    clean
    TARGET  distclean
  )

  ADD_CUSTOM_COMMAND(
    COMMENT "distribution clean"
    COMMAND rm
    ARGS    -Rf ${DISTCLEANED}
    TARGET  distclean
  )
ENDIF(UNIX)

