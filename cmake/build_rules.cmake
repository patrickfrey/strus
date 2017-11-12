# -----------------------------------------------------------------------------------------------
# Defines the flags for compiler and linker and some build environment settings
# -----------------------------------------------------------------------------------------------
# The following definition requires CMake >= 3.1
set( CMAKE_CXX_COMPILE_FEATURES 
	"cxx_long_long_type"     # because of boost using 'long long' 
)
# Temporary hack to build without warnings with CMake < 3.1:
IF (CPP_LANGUAGE_VERSION STREQUAL "0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "98")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
ENDIF (CPP_LANGUAGE_VERSION STREQUAL "0x")

IF (C_LANGUAGE_VERSION STREQUAL "99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
ENDIF (C_LANGUAGE_VERSION STREQUAL "99")

MESSAGE( STATUS "Debug postfix: '${CMAKE_DEBUG_POSTFIX}'" )

set_property(GLOBAL PROPERTY rule_launch_compile ccache)
set_property(GLOBAL PROPERTY rule_launch_link ccache)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wshadow -pedantic -Wfatal-errors -fvisibility=hidden" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors" )
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "[cC]lang")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wshadow -pedantic -Wfatal-errors -fvisibility=hidden -Wno-unused-private-field" )
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

foreach(flag ${CXX11_FEATURE_LIST})
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${flag} -pthread" )
endforeach()

if(APPLE)
   set( CMAKE_MACOSX_RPATH ON)
   set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
   set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus"  "${Boost_LIBRARY_DIRS}" )
   set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
else(APPLE)
    set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE ) 
    set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus"  "${Boost_LIBRARY_DIRS}" )
    set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )
    set( CMAKE_NO_BUILTIN_CHRPATH TRUE )
endif(APPLE)
