# based on http://agateau.com/2009/cmake-and-make-dist-the-simple-version/
# adapted for various compressors

string(TOLOWER ${CMAKE_PROJECT_NAME} PACKAGE_NAME)
set(ARCHIVE_NAME ${PACKAGE_NAME}-${STRUS_VERSION})
IF (NOT TARGET dist)
add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist )
IF (NOT TARGET dist-bz2 )
add_custom_target(dist-bz2
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist-bz2 )
IF (NOT TARGET dist-gz )
add_custom_target(dist-gz
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | gzip > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist-gz )
IF (NOT TARGET dist-xz )
add_custom_target(dist-xz
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | xz > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.xz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist-xz )
