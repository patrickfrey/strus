include( ${CMAKE_MODULE_PATH}/find_cppcheck.cmake )

file( GLOB_RECURSE MAIN_SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/ *.cpp *.hpp )
file( GLOB_RECURSE MAIN_INCLUDE_FILES ${PROJECT_SOURCE_DIR}/include/ *.hpp )
file( GLOB_RECURSE TEST_SOURCE_FILES ${PROJECT_SOURCE_DIR}/tests/ *.cpp *.hpp )
set( ALL_SOURCE_FILES ${MAIN_SOURCE_FILES} ${MAIN_INCLUDE_FILES} ${TEST_SOURCE_FILES} )

add_custom_target( cppcheck )

macro( add_cppcheck targetname )
get_directory_property( INCLUDES  INCLUDE_DIRECTORIES )
if( CPPCHECK_EXECUTABLE )
set( _INCLUDE_FLAGS )
foreach( _INC ${INCLUDES} )
list( APPEND _INCLUDE_FLAGS  "-I${_INC}" )
endforeach( _INC ${INCLUDES} )

set( _SOURCEFILES )
foreach( _SRC ${ARGN} )
if( IS_ABSOLUTE  ${_SRC} )
list( APPEND _SOURCEFILES "${_SRC}" )
else( IS_ABSOLUTE  ${_SRC} )
list( APPEND _SOURCEFILES "${CMAKE_CURRENT_LIST_DIR}/${_SRC}" )
endif( IS_ABSOLUTE  ${_SRC} )
endforeach( _SRC ${SOURCEFILES} )

add_custom_target(
        cppcheck_${targetname}
        COMMAND ${CPPCHECK_EXECUTABLE}
	--enable=all
	--suppress=missingIncludeSystem
        --std=c++11
	--verbose
	--check-config
	-D__cplusplus=201103
        --template  "{file}:{line}: {severity} {id}: {message}"
	${_INCLUDE_FLAGS}
        ${_SOURCEFILES}
)
add_dependencies( cppcheck  cppcheck_${targetname} )
endif (CPPCHECK_EXECUTABLE)
endmacro( add_cppcheck SOURCEFILES INCLUDES )

