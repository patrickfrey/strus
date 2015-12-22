# -----------------------------------------------------------------------------------------------
# Defines the flags for compiler and linker and some build environment settings
# -----------------------------------------------------------------------------------------------

if(CMAKE_COMPILER_IS_GNUCXX)
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
endif()

set_property(GLOBAL PROPERTY rule_launch_compile ccache)
set_property(GLOBAL PROPERTY rule_launch_link ccache)

if(CMAKE_COMPILER_IS_GNUCXX)
set( STRUS_OPTIMIZATION_LEVEL "0" )
set( CMAKE_CXX_FLAGS "-std=c++98  -Wall -pedantic -g -Wfatal-errors -fvisibility=hidden -fPIC -O${STRUS_OPTIMIZATION_LEVEL}" )
set( CMAKE_C_FLAGS "-std=c99 -Wall -pedantic -Wfatal-errors -fPIC -O${STRUS_OPTIMIZATION_LEVEL}" )
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0504")
endif()

