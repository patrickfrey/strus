cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( MetaDataRestrictions ${CMAKE_CURRENT_BINARY_DIR}/src/testMetaDataRestrictions 50 50 50 )
	
