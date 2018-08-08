# original from https://github.com/justmoon/bzing/blob/master/cmake/FindLevelDB.cmake, adapted

if( NOT LEVELDB_ROOT)
if( APPLE)
execute_process( COMMAND  brew  --prefix  leveldb
			   RESULT_VARIABLE  RET_LEVELDB_PATH
			   OUTPUT_VARIABLE  LEVELDB_INSTALL_PATH
			   OUTPUT_STRIP_TRAILING_WHITESPACE )
if( ${RET_LEVELDB_PATH} STREQUAL "" OR ${RET_LEVELDB_PATH} STREQUAL "0" )
set( LEVELDB_ROOT ${LEVELDB_INSTALL_PATH} )
endif( ${RET_LEVELDB_PATH} STREQUAL "" OR ${RET_LEVELDB_PATH} STREQUAL "0" )
endif( APPLE )
endif (NOT LEVELDB_ROOT)

if( LEVELDB_ROOT )
MESSAGE( STATUS "Installation path of leveldb: '${LEVELDB_ROOT}' " )

find_path( pt NAMES  leveldb/db.h
			HINTS "${LEVELDB_ROOT}/include"
			NO_CMAKE_ENVIRONMENT_PATH
			NO_CMAKE_PATH
			NO_SYSTEM_ENVIRONMENT_PATH
			NO_CMAKE_SYSTEM_PATH )
MESSAGE( STATUS "Find path leveldb/db.h no system path returns: '${pt}' " )
if( pt  AND NOT pt STREQUAL "pt-NOTFOUND" )
set( LevelDB_INCLUDE_PATH ${pt} )
endif( pt  AND NOT pt STREQUAL "pt-NOTFOUND" )
endif( LEVELDB_ROOT )

if( NOT LevelDB_INCLUDE_PATH )
find_path( pt NAMES leveldb/db.h 
			HINTS  "${CMAKE_INSTALL_PREFIX}/include"  "${CMAKE_INSTALL_PREFIX}/include/strus")
MESSAGE( STATUS "Find path leveldb/db.h with system path returns: '${pt}' " )
if( pt  AND NOT pt STREQUAL "pt-NOTFOUND" )
set( LevelDB_INCLUDE_PATH ${pt} )
endif( pt  AND NOT pt STREQUAL "pt-NOTFOUND" )
endif( NOT LevelDB_INCLUDE_PATH )

if( LEVELDB_ROOT AND NOT LevelDB_INCLUDE_PATH)
file( GLOB_RECURSE fl "${LEVELDB_ROOT}/*/db.h" )
MESSAGE( STATUS "Find path db.h recursive returns: '${fl}' " )
if( fl AND NOT "${fl}" STREQUAL "" )
list( GET fl 0 fl0 )
get_filename_component( fdir ${fl0} DIRECTORY )
get_filename_component( LevelDB_INCLUDE_PATH  ${fdir} DIRECTORY )
else ( fl AND NOT "${fl}" STREQUAL "" )
message( STATUS "recursive search for ${LEVELDB_ROOT}/*/db.h returns nothing" )
endif ( fl AND NOT "${fl}" STREQUAL "" )
endif( LEVELDB_ROOT AND NOT LevelDB_INCLUDE_PATH )

if( LEVELDB_ROOT )
find_library( pl NAMES leveldb
			HINTS "${LEVELDB_ROOT}/${LIB_INSTALL_DIR}" "${LEVELDB_ROOT}/lib"
			NO_DEFAULT_PATH
			NO_CMAKE_ENVIRONMENT_PATH
			NO_CMAKE_PATH
          		NO_SYSTEM_ENVIRONMENT_PATH
			NO_CMAKE_SYSTEM_PATH )
if( pl AND NOT pl STREQUAL "pl-NOTFOUND" )
set( LevelDB_LIBRARY ${pl} )
endif( pl AND NOT pl STREQUAL "pl-NOTFOUND" )
endif( LEVELDB_ROOT )

if( NOT LevelDB_LIBRARY )
find_library( pl NAMES leveldb
			HINTS "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}"
					 "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
if( pl AND NOT pl STREQUAL "pl-NOTFOUND" )
set( LevelDB_LIBRARY ${pl} )
endif( pl AND NOT pl STREQUAL "pl-NOTFOUND" )
endif( NOT LevelDB_LIBRARY )

if( LEVELDB_ROOT AND NOT LevelDB_LIBRARY )
if( APPLE )
file( GLOB_RECURSE al "${LEVELDB_ROOT}/*/libleveldb.dylib" )
else( APPLE )
file( GLOB_RECURSE al "${LEVELDB_ROOT}/*/libleveldb.so" )
endif( APPLE )
if( al AND NOT "${al}" STREQUAL "" )
list( GET al 0 LevelDB_LIBRARY )
else( al AND NOT "${al}" STREQUAL "" )
message( STATUS "recursive search for ${LEVELDB_ROOT}/*/libleveldb.so/dylib returns nothing" )
endif ( al AND NOT "${al}" STREQUAL "" )
endif( LEVELDB_ROOT AND NOT LevelDB_LIBRARY )

if(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY)
  set(LevelDB_FOUND TRUE)
  get_filename_component( LevelDB_LIBRARY_PATH  ${LevelDB_LIBRARY}  PATH )
  get_filename_component( LevelDB_LIBRARY  ${LevelDB_LIBRARY}  NAME )
else(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY)
  if (LevelDB_INCLUDE_PATH)
     MESSAGE( STATUS "Found include path of leveldb: '${LevelDB_INCLUDE_PATH}' " )
  else (LevelDB_INCLUDE_PATH)
     MESSAGE( WARNING "Include path of leveldb not found" )
  endif (LevelDB_INCLUDE_PATH)
  if (LevelDB_LIBRARY)
     MESSAGE( STATUS "Found library of leveldb: '${LevelDB_LIBRARY}' " )
  else (LevelDB_LIBRARY)
     MESSAGE( WARNING "Library path of leveldb not found" )
  endif (LevelDB_LIBRARY)
endif(LevelDB_INCLUDE_PATH AND LevelDB_LIBRARY)

if(LevelDB_FOUND)
  if(NOT LevelDB_FIND_QUIETLY)
    message( STATUS "Found LevelDB")
    message( STATUS "LevelDB include path: ${LevelDB_INCLUDE_PATH}")
    message( STATUS "LevelDB library path: ${LevelDB_LIBRARY_PATH}")
    message( STATUS "LevelDB library: ${LevelDB_LIBRARY}")
  endif(NOT LevelDB_FIND_QUIETLY)
else(LevelDB_FOUND)
  if(LevelDB_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find leveldb library.")
  endif(LevelDB_FIND_REQUIRED)
endif(LevelDB_FOUND)
