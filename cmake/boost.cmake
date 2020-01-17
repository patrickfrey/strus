set(Boost_USE_MULTITHREADED ON)

# Some libraries need in different version in a multithreaded context on OSX and Windows. The multithreaded version has a defined name suffix.
# Define a gloval variable for the suffix to use in such a case:
if( APPLE OR WIN32 )
set( MT_SUFFIX  "-mt" )
else( APPLE OR WIN32 )
set( MT_SUFFIX  "" )
endif( APPLE OR WIN32 )

# Initialize lists of components and libraries
set( Boost_LIBRARIES  "" )
set( Boost_COMPONENTS  "" )

# Function to declare a library to use and its corresponding component name for finding it in case of a boost library:
FUNCTION( boost_library  compname  _suffix )
if( "${compname}" MATCHES  "boost_(.+)" )
set( Boost_COMPONENTS ${Boost_COMPONENTS} "${CMAKE_MATCH_1}${_suffix}" PARENT_SCOPE )
endif( "${compname}" MATCHES  "boost_(.+)" )
set( Boost_LIBRARIES  ${Boost_LIBRARIES}  "${compname}${_suffix}"  PARENT_SCOPE )
ENDFUNCTION( boost_library compname )

# Declare components:
boost_library( boost_thread  "${MT_SUFFIX}" )
boost_library( pthread "" )
boost_library( boost_chrono  "${MT_SUFFIX}" )
boost_library( boost_system  "" )
boost_library( boost_date_time  "" )
boost_library( boost_atomic  "${MT_SUFFIX}" )
if( NOT HAS_CXX11_REGEX )
message( STATUS "Has C++ std::regex, don't use boost regex" )
boost_library( boost_regex  "" )
endif( NOT HAS_CXX11_REGEX )

# Declare or try lookup the installation path of boost:
if( BOOST_ROOT )
MESSAGE( STATUS "Boost root set: '${BOOST_ROOT}' " )
set( BOOST_INSTALL_PATH ${BOOST_ROOT} )
else( BOOST_ROOT )
if (APPLE)
execute_process( COMMAND  brew  --prefix  boost
			   RESULT_VARIABLE  RET_BOOST_PATH
			   OUTPUT_VARIABLE  OUTPUT_BOOST_PATH
			   OUTPUT_STRIP_TRAILING_WHITESPACE )
if( ${RET_BOOST_PATH} STREQUAL "" OR ${RET_BOOST_PATH} STREQUAL "0" )
MESSAGE( STATUS "Call brew  --prefix  boost result: '${OUTPUT_BOOST_PATH}' " )
set( BOOST_INSTALL_PATH ${OUTPUT_BOOST_PATH} )
else( ${RET_BOOST_PATH} STREQUAL "" OR ${RET_BOOST_PATH} STREQUAL "0" )
MESSAGE( STATUS "Call brew  --prefix  boost failed with error: '${RET_BOOST_PATH}' " )
endif( ${RET_BOOST_PATH} STREQUAL "" OR ${RET_BOOST_PATH} STREQUAL "0" )
endif (APPLE)
endif (BOOST_ROOT)

if( BOOST_INSTALL_PATH )
MESSAGE( STATUS "Boost installation path: '${BOOST_INSTALL_PATH}' " )
set( Boost_LIBRARY_DIRS "${BOOST_INSTALL_PATH}/lib" )
set( Boost_INCLUDE_DIRS "${BOOST_INSTALL_PATH}/include" )

else( BOOST_INSTALL_PATH )

# If no boost installation path declared then use the cmake standard module for boost to find the package and to resolve the paths:
if( BOOST_ROOT )
MESSAGE( STATUS "Search for Boost in BOOST_ROOT=${BOOST_ROOT}" )
find_package( Boost 1.57.0 COMPONENTS atomic QUIET HINTS "${BOOST_ROOT}" )
else( BOOST_ROOT )
MESSAGE( STATUS "Search for Boost ..." )
find_package( Boost 1.57.0 COMPONENTS atomic QUIET)
endif( BOOST_ROOT )

if( Boost_ATOMIC_FOUND )
        MESSAGE( STATUS "Found boost installation, search now for required packages ..." )
	find_package( Boost 1.57.0 REQUIRED COMPONENTS ${Boost_COMPONENTS} )
else( Boost_ATOMIC_FOUND )
        MESSAGE( WARNING "No boost installation found !" )
endif( Boost_ATOMIC_FOUND )

endif( BOOST_INSTALL_PATH )

# Report initializations for using boost:
MESSAGE( STATUS "Boost components: ${Boost_COMPONENTS}" )
MESSAGE( STATUS "Boost includes: ${Boost_INCLUDE_DIRS}" )
MESSAGE( STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}" )
MESSAGE( STATUS "Boost libraries: ${Boost_LIBRARIES}" )

