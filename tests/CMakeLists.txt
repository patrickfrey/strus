cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
add_subdirectory( loggingMalloc )
endif()
add_subdirectory( scalarfunc )
add_subdirectory( storageop )
add_subdirectory( insert )
add_subdirectory( struct )
add_subdirectory( queryop )
add_subdirectory( query )
add_subdirectory( queryeval )
add_subdirectory( functions )
add_subdirectory( metaDataRestrictions )
add_subdirectory( ranker )
add_subdirectory( merger )
add_subdirectory( booleanBlock )
add_subdirectory( posinfoBlock )
add_subdirectory( positionWindow )
add_subdirectory( randoc )
add_subdirectory( varSizeNodeTree )
add_subdirectory( statsproc )
add_subdirectory( weightingTitle )
add_subdirectory( weighting )

