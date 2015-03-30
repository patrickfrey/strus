cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( libstrus_storage_source_files
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
	storageAlterMetaDataTable.cpp
	storage.cpp
	storageClient.cpp
	storageDocumentChecker.cpp
	storageDocument.cpp
	storageTransaction.cpp
	userAclMap.cpp
	extractKeyValueData.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/include"  )
link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${PROJECT_SOURCE_DIR}/utils"
	"${PROJECT_SOURCE_DIR}/database_leveldb"
	"${PROJECT_SOURCE_DIR}/3rdParty/compactNodeTrie/src"  )

# ------------------------------
# STORAGE LIBRARY
# Build a shared library as deployment artefact and a static library for tests that need to bypass the library interface to do their job
# ------------------------------
add_library( strus_storage_static STATIC ${libstrus_storage_source_files} )
target_link_libraries( strus_storage_static strus_private_utils strus_utils )

add_library(strus_storage SHARED libstrus_storage.cpp )
target_link_libraries( strus_storage strus_storage_static boost_system strus_private_utils strus_database_leveldb compactnodetrie_static strus_utils )
set_target_properties(
    strus_storage
    PROPERTIES
    DEBUG_POSTFIX "_d"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_storage
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus
	   RUNTIME DESTINATION bin )

