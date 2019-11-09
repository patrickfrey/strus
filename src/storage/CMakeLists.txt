cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	attributeMap.cpp
	attributeReader.cpp
	aclReader.cpp
	booleanBlockBatchWrite.cpp
	booleanBlock.cpp
	databaseAdapter.cpp
	databaseKey.cpp
	dataBlock.cpp
	documentFrequencyCache.cpp
	documentFrequencyMap.cpp
	floatConversions.cpp
	uintCompaction.cpp
	forwardIndexBlock.cpp
	forwardIndexMap.cpp
	forwardIterator.cpp
	documentTermIterator.cpp
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
	metaDataReader.cpp
	metaDataRestriction.cpp
	nullPostingIterator.cpp
	browsePostingIterator.cpp
	statisticsInitIterator.cpp
	docIndexNode.cpp
	posinfoBlock.cpp
	posinfoIterator.cpp
	postingIterator.cpp
	structBlock.cpp
	structIndexMap.cpp
	structIterator.cpp
	storageMetaDataTableUpdate.cpp
	storageMetaDataTransaction.cpp
	storage.cpp
	storageClient.cpp
	storageDocumentChecker.cpp
	storageDocument.cpp
	storageDocumentUpdate.cpp
	storageTransaction.cpp
	storageDump.cpp
	userAclMap.cpp
	extractKeyValueData.cpp
)

include_directories(
	${Boost_INCLUDE_DIRS}
	"${Intl_INCLUDE_DIRS}"
	"${STRUS_INCLUDE_DIRS}"
	"${CNODETRIE_INCLUDE_DIRS}" 
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	${Boost_LIBRARY_DIRS}
	"${MAIN_SOURCE_DIR}/utils"
	"${MAIN_SOURCE_DIR}/database_leveldb"
	"${MAIN_SOURCE_DIR}/statsproc"
	"${MAIN_SOURCE_DIR}/sentence"
	"${CNODETRIE_LIBRARY_DIRS}" 
	"${strusbase_LIBRARY_DIRS}"
	"${LevelDB_LIBRARY_PATH}"
)

# ------------------------------
# STORAGE LIBRARY
# Build a shared library as deployment artefact and a static library for tests that need to bypass the library interface to do their job
# ------------------------------
add_cppcheck( strus_storage_static ${source_files} libstrus_storage.cpp )

add_library( strus_storage_static STATIC ${source_files} )
target_link_libraries( strus_storage_static strus_private_utils strus_base )
set_property( TARGET strus_storage_static PROPERTY POSITION_INDEPENDENT_CODE TRUE )

add_library( strus_storage SHARED libstrus_storage.cpp )
target_link_libraries( strus_storage strus_storage_static ${Boost_LIBRARIES} strus_private_utils strus_database_leveldb strus_statsproc compactnodetrie_strus_static strus_base )
set_target_properties(
    strus_storage
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

add_library( strus_storage_objbuild SHARED libstrus_storage_objbuild.cpp )
target_link_libraries( strus_storage_objbuild strus_storage strus_queryeval strus_queryproc ${Boost_LIBRARIES} strus_private_utils strus_database_leveldb strus_statsproc strus_sentence compactnodetrie_strus_static strus_base )
set_target_properties(
    strus_storage_objbuild
    PROPERTIES
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
    SOVERSION "${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}"
    VERSION ${STRUS_VERSION}
)

add_executable( strusResizeBlocks strusResizeBlocks.cpp )
target_link_libraries( strusResizeBlocks  "${Boost_LIBRARIES}" strus_database_leveldb strus_private_utils strus_error strus_base strus_filelocator strus_storage_static compactnodetrie_strus_static ${Intl_LIBRARIES})

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strus_storage
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus
	   RUNTIME DESTINATION bin )

install( TARGETS strus_storage_objbuild
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus
	   RUNTIME DESTINATION bin )

install( TARGETS strusResizeBlocks
	   RUNTIME DESTINATION bin )

