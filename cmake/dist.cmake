# based on http://agateau.com/2009/cmake-and-make-dist-the-simple-version/
# adapted for various compressors

string(TOLOWER ${CMAKE_PROJECT_NAME} PACKAGE_NAME)
set(ARCHIVE_NAME ${PACKAGE_NAME}-${STRUS_VERSION})
IF (NOT TARGET dist)
add_custom_target(dist
    COMMAND ${CMAKE_SOURCE_DIR}/dist/helpers/git-archive-all.sh --prefix ${ARCHIVE_NAME}/ -t HEAD
        --format tar ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist )
IF (NOT TARGET dist-gz )
add_custom_target(dist-gz
    COMMAND ${CMAKE_SOURCE_DIR}/dist/helpers/git-archive-all.sh --prefix ${ARCHIVE_NAME}/ -t HEAD
        --format tar.gz ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (NOT TARGET dist-gz )
