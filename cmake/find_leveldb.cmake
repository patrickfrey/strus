# from https://github.com/justmoon/bzing/blob/master/cmake/FindLevelDB.cmake, slightly adapted

find_path(LevelDB_INCLUDE_PATH NAMES leveldb/db.h HINTS "${CMAKE_INSTALL_PREFIX}/include/strus")
find_library(LevelDB_LIBRARY NAMES leveldb HINTS "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus")

if(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY)
  set(LevelDB_FOUND TRUE)
endif(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY)

if(LevelDB_FOUND)
  if(NOT LevelDB_FIND_QUIETLY)
    message(STATUS "Found LevelDB: ${LevelDB_LIBRARY}")
    message(STATUS "LevelDB include: ${LevelDB_INCLUDE_PATH}")
    message(STATUS "LevelDB library: ${LevelDB_LIBRARY}")
  endif(NOT LevelDB_FIND_QUIETLY)
else(LevelDB_FOUND)
  if(LevelDB_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find leveldb library.")
  endif(LevelDB_FIND_REQUIRED)
endif(LevelDB_FOUND)
