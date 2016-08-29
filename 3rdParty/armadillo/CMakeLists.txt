# -----------------------------------------------------------------------------------------------------------------------------------
# Modified version of the orginal CMake file with different installation section as local version bound to Strus:
# -----------------------------------------------------------------------------------------------------------------------------------

# 
# Copyright (C) 2008-2016 National ICT Australia (NICTA)
# 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# -------------------------------------------------------------------
# 
# Written by Conrad Sanderson - http://conradsanderson.id.au
# Written by Ryan Curtin
# Written by Clement Creusot



cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(ARMA_MAJOR 7)
set(ARMA_MINOR 400)
set(ARMA_PATCH 2)
set(ARMADILLO_VERSION ${ARMA_MAJOR}.${ARMA_MINOR}.${ARMA_PATCH})

message(STATUS "Configuring local Armadillo for Strus: ${ARMADILLO_VERSION}")


## Set ARMA_USE_WRAPPER to false if you're getting linking errors when compiling your programs,
## or if you prefer to directly link with BLAS and/or LAPACK.
## You will then need to link your programs with -lblas -llapack instead of -larmadillo
## If you're using OpenBLAS, link your programs with -lopenblas -llapack instead of -larmadillo

set(ARMA_USE_WRAPPER true)


# the settings below will be automatically configured by the rest of this script

set(ARMA_USE_LAPACK           false)
set(ARMA_USE_BLAS             false)
set(ARMA_USE_ATLAS            false)
set(ARMA_USE_HDF5_ALT         false)
set(ARMA_USE_ARPACK           false)
set(ARMA_USE_EXTERN_CXX11_RNG false)
set(ARMA_USE_SUPERLU          false)  # Caveat: only SuperLU version 5.x can be used!

project(armadillo CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_aux/Modules/")

include(CheckIncludeFileCXX)
include(CheckLibraryExists)


if(MSVC)
  option(BUILD_SHARED_LIBS "build shared library" OFF)
else()
  option(BUILD_SHARED_LIBS "build shared library" ON)
endif()

option(DETECT_HDF5 "Detect HDF5 and include HDF5 support, if found" OFF)


if(WIN32)
  message(STATUS "")
  message(STATUS "*** WARNING: automatic installation is experimental for this platform.")
  message(STATUS "*** WARNING: if anything breaks, you get to keep all the pieces.")
  message(STATUS "*** WARNING: manual installation is described in README.txt")
  message(STATUS "")
endif()

if(MSVC AND BUILD_SHARED_LIBS)
  message(STATUS "")
  message(STATUS "*** WARNING: building shared library with MSVC is not supported." )
  message(STATUS "*** WARNING: if anything breaks, you get to keep all the pieces.")
  message(STATUS "")
endif()


message(STATUS "CMAKE_SYSTEM_NAME          = ${CMAKE_SYSTEM_NAME}"         )
message(STATUS "CMAKE_CXX_COMPILER_ID      = ${CMAKE_CXX_COMPILER_ID}"     )
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_COMPILER_IS_GNUCXX   = ${CMAKE_COMPILER_IS_GNUCXX}"  )
message(STATUS "BUILD_SHARED_LIBS          = ${BUILD_SHARED_LIBS}"         )
message(STATUS "DETECT_HDF5                = ${DETECT_HDF5}"               )


##
## Find LAPACK and BLAS libraries, or their optimised versions
##

if(APPLE)
  
  set(ARMA_OS macos)
  
  set(ARMA_USE_LAPACK true)
  set(ARMA_USE_BLAS   true)
  
  set(ARMA_LIBS ${ARMA_LIBS} "-framework Accelerate")  # or "-framework accelerate" ?
  message(STATUS "MacOS X detected. Added '-framework Accelerate' to compiler flags")
  
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    message(STATUS "Clang compiler on MacOS X detected. Added '-stdlib=libc++' to compiler flags")
  endif()
  
else()
  
  set(ARMA_OS unix)
  
  include(ARMA_FindMKL)
  include(ARMA_FindACMLMP)
  include(ARMA_FindACML)
  include(ARMA_FindOpenBLAS)
  include(ARMA_FindATLAS)
  include(ARMA_FindBLAS)
  include(ARMA_FindLAPACK)
  
  message(STATUS "     MKL_FOUND = ${MKL_FOUND}"     )
  message(STATUS "  ACMLMP_FOUND = ${ACMLMP_FOUND}"  )
  message(STATUS "    ACML_FOUND = ${ACML_FOUND}"    )
  message(STATUS "OpenBLAS_FOUND = ${OpenBLAS_FOUND}")
  message(STATUS "   ATLAS_FOUND = ${ATLAS_FOUND}"   )
  message(STATUS "    BLAS_FOUND = ${BLAS_FOUND}"    )
  message(STATUS "  LAPACK_FOUND = ${LAPACK_FOUND}"  )
  
  if(MKL_FOUND OR ACMLMP_FOUND OR ACML_FOUND)
    
    set(ARMA_USE_LAPACK true)
    set(ARMA_USE_BLAS   true)
    
    message(STATUS "")
    message(STATUS "*** If the MKL or ACML libraries are installed in non-standard locations such as")
    message(STATUS "*** /opt/intel/mkl, /opt/intel/composerxe/, /usr/local/intel/mkl")
    message(STATUS "*** make sure the run-time linker can find them.")
    message(STATUS "*** On Linux systems this can be done by editing /etc/ld.so.conf")
    message(STATUS "*** or modifying the LD_LIBRARY_PATH environment variable.")
    message(STATUS "")
    message(STATUS "*** On systems with SELinux enabled (eg. Fedora, RHEL),")
    message(STATUS "*** you may need to change the SELinux type of all MKL/ACML libraries")
    message(STATUS "*** to fix permission problems that may occur during run-time.")
    message(STATUS "*** See README.txt for more information")
    message(STATUS "")
    
    if(MKL_FOUND)
      set(ARMA_LIBS ${ARMA_LIBS} ${MKL_LIBRARIES})
      
      if(ACMLMP_FOUND OR ACML_FOUND)
        message(STATUS "*** Intel MKL as well as AMD ACML libraries were found.")
        message(STATUS "*** Using only the MKL library to avoid linking conflicts.")
        message(STATUS "*** If you wish to use ACML instead, please link manually with")
        message(STATUS "*** acml or acml_mp instead of the armadillo wrapper library.")
        message(STATUS "*** Alternatively, remove MKL from your system and rerun")
        message(STATUS "*** Armadillo's configuration using ./configure") 
      endif()
      
    else()
      
      if(ACMLMP_FOUND)
        set(ARMA_LIBS ${ARMA_LIBS} ${ACMLMP_LIBRARIES})
        
        message(STATUS "*** Both single-core and multi-core ACML libraries were found.")
        message(STATUS "*** Using only the multi-core library to avoid linking conflicts.")
      else()
        if(ACML_FOUND)
          set(ARMA_LIBS ${ARMA_LIBS} ${ACML_LIBRARIES})
        endif()
      endif()
      
    endif()
    
  else()
    
    if(OpenBLAS_FOUND AND ATLAS_FOUND)
      message(STATUS "")
      message(STATUS "*** WARNING: found both OpenBLAS and ATLAS; ATLAS will not be used")
    endif()
    
    if(OpenBLAS_FOUND AND BLAS_FOUND)
      message(STATUS "")
      message(STATUS "*** WARNING: found both OpenBLAS and BLAS; BLAS will not be used")
    endif()
    
    if(OpenBLAS_FOUND)
      
      set(ARMA_USE_BLAS true)
      set(ARMA_LIBS ${ARMA_LIBS} ${OpenBLAS_LIBRARIES})
      
      message(STATUS "")
      message(STATUS "*** If the OpenBLAS library is installed in")
      message(STATUS "*** /usr/local/lib or /usr/local/lib64")
      message(STATUS "*** make sure the run-time linker can find it.")
      message(STATUS "*** On Linux systems this can be done by editing /etc/ld.so.conf")
      message(STATUS "*** or modifying the LD_LIBRARY_PATH environment variable.")
      message(STATUS "")
      
    else()
      
      if(ATLAS_FOUND)
        set(ARMA_USE_ATLAS true)
        set(ARMA_ATLAS_INCLUDE_DIR ${ATLAS_INCLUDE_DIR})
        set(ARMA_LIBS ${ARMA_LIBS} ${ATLAS_LIBRARIES})
        
        message(STATUS "ATLAS_INCLUDE_DIR = ${ATLAS_INCLUDE_DIR}")
      endif()
      
      if(BLAS_FOUND)
        set(ARMA_USE_BLAS true)
        set(ARMA_LIBS ${ARMA_LIBS} ${BLAS_LIBRARIES})
      endif()
      
    endif()
    
    if(LAPACK_FOUND)
      set(ARMA_USE_LAPACK true)
      set(ARMA_LIBS ${ARMA_LIBS} ${LAPACK_LIBRARIES})
    endif()
      
  endif()
  
endif()


# set(DETECT_HDF5 true)

## uncomment the above line to enable the detection of the HDF5 library;
## you can also enable HDF5 detection directly on the command line:
## cmake -D DETECT_HDF5=true .

if(DETECT_HDF5)
  find_package(HDF5 QUIET)

  if(NOT HDF5_FOUND)
    # On Debian systems, the HDF5 package has been split into multiple packages
    # so that it is co-installable.  But this may mean that the include files
    # are hidden somewhere very odd that the FindHDF5.cmake script will not
    # find.  Thus, we'll also quickly check pkgconfig to see if there is
    # information on what to use there.
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(HDF5 hdf5)
      # But using pkgconfig is a little weird because HDF5_LIBRARIES won't be
      # filled with exact library paths, like the other scripts.  So instead
      # what we get is HDF5_LIBRARY_DIRS which is the equivalent of what we'd
      # pass to -L.  So we have to add those...
      if (HDF5_FOUND)
        link_directories("${HDF5_LIBRARY_DIRS}")
      endif()
    endif()
  endif()
 
  message(STATUS "HDF5_FOUND = ${HDF5_FOUND}")
  if(HDF5_FOUND)
    set(ARMA_USE_HDF5_ALT true)
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${HDF5_INCLUDE_DIRS})
    set(ARMA_LIBS ${ARMA_LIBS} ${HDF5_LIBRARIES})
    # Since we called HDF5 with no arguments, the script will find only the C
    # bindings.  So HDF5_INCLUDE_DIRS may now contain one or two elements; if it
    # contains two, the first is what the user passed as HDF5_INCLUDE_DIR and we
    # should use that as ARMA_HDF5_INCLUDE_DIR.  Otherwise, the one entry in
    # HDF5_INCLUDE_DIRS is the correct include directory.  So, in either case we
    # can use the first element in the list.  Issue a status message, too, just
    # for good measure.
    list(GET HDF5_INCLUDE_DIRS 0 ARMA_HDF5_INCLUDE_DIR)
    message(STATUS "Set ARMA_HDF5_INCLUDE_DIR to ${ARMA_HDF5_INCLUDE_DIR}")
  endif()
endif()


include(ARMA_FindARPACK)
message(STATUS "ARPACK_FOUND = ${ARPACK_FOUND}")

if(ARPACK_FOUND)
  set(ARMA_USE_ARPACK true)
  set(ARMA_LIBS ${ARMA_LIBS} ${ARPACK_LIBRARY})
endif()

include(ARMA_FindSuperLU5)
message(STATUS "SuperLU_FOUND = ${SuperLU_FOUND}")

if(SuperLU_FOUND)
  message(STATUS "SuperLU_INCLUDE_DIR = ${SuperLU_INCLUDE_DIR}")
  set(ARMA_USE_SUPERLU true)
  set(ARMA_LIBS ${ARMA_LIBS} ${SuperLU_LIBRARY})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${SuperLU_INCLUDE_DIR})
  set(ARMA_SUPERLU_INCLUDE_DIR ${SuperLU_INCLUDE_DIR})
endif()

message(STATUS "")
message(STATUS "*** Armadillo wrapper library will use the following libraries:")
message(STATUS "*** ARMA_LIBS = ${ARMA_LIBS}")
message(STATUS "")


if (WITH_CPP11 STREQUAL "YES")
if(DEFINED CMAKE_CXX_COMPILER_ID AND DEFINED CMAKE_CXX_COMPILER_VERSION)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.8.3)
      set(ARMA_USE_EXTERN_CXX11_RNG true)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
      message(STATUS "Detected gcc 4.8.3 or later. Added '-std=c++11' to compiler flags")
  endif()
endif()
else()
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
      message( STATUS "Set flag WITH_CPP11='NO'. Added '-std=c++98' to compiler flags" )
endif()

message(STATUS "Generating ${PROJECT_BINARY_DIR}/tmp/include/config.hpp")
configure_file(${PROJECT_SOURCE_DIR}/include/armadillo_bits/config.hpp.cmake ${PROJECT_BINARY_DIR}/tmp/include/armadillo_bits/config.hpp)

include_directories( ${PROJECT_SOURCE_DIR}/include ${CMAKE_REQUIRED_INCLUDES})


## work around a silly limitation in Mac OS X
if(APPLE)
  if(${ARMA_MINOR} GREATER 99)
    math(EXPR ARMA_MINOR_ALT "${ARMA_MINOR} / 10")
  else()
    set(ARMA_MINOR_ALT ${ARMA_MINOR})
  endif()
else()
  set(ARMA_MINOR_ALT ${ARMA_MINOR})
endif()


## necessary when linking with Intel MKL on Linux systems
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")
endif()


if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

message(STATUS "CMAKE_CXX_FLAGS           = ${CMAKE_CXX_FLAGS}"          )
message(STATUS "CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "CMAKE_REQUIRED_INCLUDES   = ${CMAKE_REQUIRED_INCLUDES}"  )


add_library( strus_armadillo ${PROJECT_SOURCE_DIR}/src/wrapper.cpp )
target_link_libraries( strus_armadillo ${ARMA_LIBS} )
set_target_properties( strus_armadillo PROPERTIES VERSION ${ARMA_MAJOR}.${ARMA_MINOR_ALT}.${ARMA_PATCH} SOVERSION ${ARMA_MAJOR})

install(DIRECTORY include/armadillo_bits
          DESTINATION include/strus
          FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY include/
          DESTINATION include/strus
          FILES_MATCHING PATTERN "armadillo")
install( TARGETS strus_armadillo
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )


