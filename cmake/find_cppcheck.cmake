cmake_minimum_required( VERSION 2.8 FATAL_ERROR )

find_program( CPPCHECK NAMES  "cppcheck" )
if( NOT CPPCHECK OR ${CPPCHECK} STREQUAL "cppcheck-NOTFOUND" )
MESSAGE( STATUS "No cppcheck program found, target cppcheck will not be provided" )
else( NOT CPPCHECK OR ${CPPCHECK} STREQUAL "cppcheck-NOTFOUND" )
set( CPPCHECK_EXECUTABLE ${CPPCHECK} )
MESSAGE( STATUS "Program cppcheck found: ${CPPCHECK_EXECUTABLE}, target cppcheck provided" )
endif( NOT CPPCHECK OR ${CPPCHECK} STREQUAL "cppcheck-NOTFOUND" )
