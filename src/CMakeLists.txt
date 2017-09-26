cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory( utils )
add_subdirectory( database_leveldb )
add_subdirectory( storage )
add_subdirectory( queryproc )
add_subdirectory( queryeval )
add_subdirectory( statsproc )
add_subdirectory( scalarfunc )
