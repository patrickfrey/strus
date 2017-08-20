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

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors -fvisibility=hidden" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors" )
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "[cC]lang")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors -fvisibility=hidden -Wno-unused-private-field" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors" )
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0504")
else()
set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
find_package( Threads REQUIRED )
if(CMAKE_USE_PTHREADS_INIT)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread" )
endif()
endif()

if(APPLE)
   set(CMAKE_MACOSX_RPATH ON)
   set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
   set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus" )
   set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
endif(APPLE)

foreach(flag ${CXX11_FEATURE_LIST})
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${flag} -pthread" )
endforeach()

