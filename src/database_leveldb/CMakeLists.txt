cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_database_leveldb_source_files
	levelDbHandle.cpp
	database.cpp
	databaseClient.cpp
	databaseCursor.cpp
	databaseTransaction.cpp
	libstrus_database_leveldb.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${LevelDB_INCLUDE_PATH}"
	"${GETMEMSIZE_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	${Boost_LIBRARY_DIRS}
	"${MAIN_SOURCE_DIR}/utils"
	"${GETMEMSIZE_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
)

# -------------------------------------------
# DATABASE LIBRARY
# -------------------------------------------
add_library( strus_database_leveldb SHARED ${libstrus_database_leveldb_source_files} )
target_link_libraries( strus_database_leveldb ${Boost_LIBRARIES} strus_base getmemsize_static strus_private_utils ${LevelDB_LIBRARY})
set_target_properties(
    strus_database_leveldb
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_database_leveldb
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

