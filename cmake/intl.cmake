function(CheckHasModule Module)
  find_package(${Module} QUIET)
  if(NOT DEFINED ${Module}_DIR)
    set(HAS_MODULE_${Module} TRUE PARENT_SCOPE)
  elseif(${Module}_DIR)
    set(HAS_MODULE_${Module} TRUE PARENT_SCOPE)
  else()
    set(HAS_MODULE_${Module} FALSE PARENT_SCOPE)
  endif()
endfunction()

CheckHasModule(Intl)
MESSAGE( STATUS  "CMake module for libintl exists: ${HAS_MODULE_Intl}" )
if(HAS_MODULE_Intl)
  find_package(Intl REQUIRED)
  if(FOUND_Intl)
    MESSAGE( STATUS  "libintl found." )
  endif()
else()
  # fallback for old Linux and OSX (Travis) where cmake is simply too old
  # and doesn't provide proper intl probing, we also assume that brew --link
  # was called on gettext
  if(APPLE)
    set(Intl_INCLUDE_DIRS "/usr/local/include")
    set(Intl_LIBRARIES "/usr/local/lib/libintl.dylib")
  else(APPLE)
    # avoid having empty directory warnings, include /usr/include should not harm too much for now
    set(Intl_INCLUDE_DIRS "/usr/include")
    # libintl is part of glibc
  endif(APPLE)
endif()

MESSAGE( STATUS "libintl include: ${Intl_INCLUDE_DIRS}" )
MESSAGE( STATUS "libintl libraries: ${Intl_LIBRARIES}" )

find_package(Gettext REQUIRED)
