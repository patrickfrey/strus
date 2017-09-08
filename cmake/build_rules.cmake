# -----------------------------------------------------------------------------------------------
# Defines the flags for compiler and linker and some build environment settings
# -----------------------------------------------------------------------------------------------
# The following definition requires CMake >= 3.1
set( CMAKE_CXX_COMPILE_FEATURES 
	"cxx_long_long_type"     # because of boost using 'long long' 
)
# Temporary hack to build without warnings with CMake < 3.1:
IF (CPP_LANGUAGE_VERSION STREQUAL "0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "98")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall")
ENDIF (CPP_LANGUAGE_VERSION STREQUAL "0x")

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

