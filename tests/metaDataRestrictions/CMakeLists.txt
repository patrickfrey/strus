cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( MetaDataRestrictions src/testMetaDataRestrictions 50 50 50 )
	
