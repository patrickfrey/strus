cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(libstrus_storage_source_files
attributeMap.cpp
attributeReader.cpp
booleanBlockBatchWrite.cpp
booleanBlock.cpp
databaseAdapter.cpp
databaseKey.cpp
dataBlock.cpp
documentFrequencyCache.cpp
documentFrequencyMap.cpp
floatConversions.cpp
forwardIndexBlock.cpp
forwardIndexMap.cpp
forwardIterator.cpp
indexPacker.cpp
indexSetIterator.cpp
invertedIndexMap.cpp
invTermBlock.cpp
keyMap.cpp
libstrus_storage.cpp
metaDataBlockCache.cpp
metaDataBlock.cpp
metaDataDescription.cpp
metaDataElement.cpp
metaDataMap.cpp
metaDataRecord.cpp
nullIterator.cpp
peerStorageTransaction.cpp
posinfoBlock.cpp
posinfoIterator.cpp
postingIterator.cpp
statistics.cpp
storageAlterMetaDataTable.cpp
storage.cpp
storageDocumentChecker.cpp
storageDocument.cpp
storageTransaction.cpp
userAclMap.cpp
varSizeNodeTree.cpp
)

set(program_storage_source_files
${libstrus_storage_source_files}
extractKeyValueData.cpp
)

include_directories( "${Boost_INCLUDE_DIRS}"  "${PROJECT_SOURCE_DIR}/include")
link_directories( /usr/local/lib "${PROJECT_SOURCE_DIR}/utils" "${PROJECT_SOURCE_DIR}/database_leveldb")

# ------------------------------
# STORAGE LIBRARY
# Build a shared library as deployment artefact and a static library for tests that need to bypass the library interface to do their job
# ------------------------------
add_library(strus_storage SHARED ${libstrus_storage_source_files})
target_link_libraries( strus_storage boost_system strus_utils strus_database_leveldb)
add_library(strus_storage_static STATIC ${program_storage_source_files})
set_target_properties(
    strus_storage
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# HELPER PROGRAMS
# ------------------------------
add_executable( strusDumpStorage strusDumpStorage.cpp)
target_link_libraries (strusDumpStorage boost_system strus_utils strus_database_leveldb strus_storage_static)
add_executable( strusCheckStorage strusCheckStorage.cpp)
target_link_libraries (strusCheckStorage boost_system strus_utils strus_database_leveldb strus_storage_static)


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_storage strusDumpStorage strusCheckStorage
           LIBRARY DESTINATION lib/strus
	   RUNTIME DESTINATION bin )

