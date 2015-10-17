cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_database_leveldb_source_files
   database.cpp
   databaseClient.cpp
   databaseCursor.cpp
   databaseTransaction.cpp
   libstrus_database_leveldb.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/3rdParty/getMemorySize/include"
	"${PROJECT_SOURCE_DIR}/include" )
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
	"${PROJECT_SOURCE_DIR}/3rdParty/getMemorySize/src"
	"${PROJECT_SOURCE_DIR}/database_leveldb" )


# -------------------------------------------
# DATABASE LIBRARY
# -------------------------------------------
add_library( strus_database_leveldb SHARED ${libstrus_database_leveldb_source_files} )
target_link_libraries( strus_database_leveldb boost_system strus_utils getmemsize_static strus_private_utils leveldb)
set_target_properties(
    strus_database_leveldb
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_database_leveldb
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus )

