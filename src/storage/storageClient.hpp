/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_HPP_INCLUDED
#define _STRUS_STORAGE_HPP_INCLUDED
#include "strus/storageClientInterface.hpp"
#include "strus/storage/index.hpp"
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/string_view.hpp"
#include "strus/storage/termStatistics.hpp"
#include "metaDataBlockCache.hpp"
#include "indexSetIterator.hpp"
#include "strus/statisticsProcessorInterface.hpp"

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class DocumentTermIteratorInterface;
/// \brief Forward declaration
class InvAclIteratorInterface;
/// \brief Forward declaration
class StructureIteratorInterface;
/// \brief Forward declaration
class StorageTransactionInterface;
/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class MetaDataRestrictionInterface;
/// \brief Forward declaration
class AclReaderInterface;
/// \brief Forward declaration
class KeyAllocatorInterface;
/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DocumentFrequencyCache;
/// \brief Forward declaration
class StorageDumpInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Implementation of the StorageClientInterface
class StorageClient
	:public StorageClientInterface
{
public:
	/// \param[in] database key value store database type used by this storage
	/// \param[in] statisticsProc_ statistics message processor interface
	/// \param[in] databaseConfig configuration string (not a filename!) of the database interface to create for this storage
	/// \param[in] errorhnd_ error buffering interface for error handling
	StorageClient(
			const DatabaseInterface* database_,
			const StatisticsProcessorInterface* statisticsProc_,
			const std::string& databaseConfig,
			ErrorBufferInterface* errorhnd_);
	virtual ~StorageClient();

	virtual bool reload( const std::string& config);

	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& termtype,
			const std::string& termid,
			const Index& length,
			const TermStatistics& stats) const;

	virtual PostingIteratorInterface*
		createFrequencyPostingIterator(
			const std::string& termtype,
			const std::string& termid,
			const TermStatistics& stats) const;

	virtual StructureIteratorInterface*
		createStructureIterator() const;

	virtual PostingIteratorInterface*
		createBrowsePostingIterator(
			const MetaDataRestrictionInterface* restriction,
			const Index& maxpos) const;

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type) const;

	virtual DocumentTermIteratorInterface*
			createDocumentTermIterator(
				const std::string& type) const;

	virtual InvAclIteratorInterface*
			createInvAclIterator(
				const std::string& username) const;

	virtual AclReaderInterface* createAclReader() const;

	virtual StorageTransactionInterface*
			createTransaction();

	virtual StorageDocumentInterface*
			createDocumentChecker(
				const std::string& docid,
				const std::string& logfilename) const;

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual MetaDataRestrictionInterface* createMetaDataRestriction() const;

	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual StatisticsIteratorInterface* createChangeStatisticsIterator( const TimeStamp& timestamp) const;

	virtual std::vector<TimeStamp> getChangeStatisticTimeStamps() const;

	virtual StatisticsMessage loadChangeStatisticsMessage( const TimeStamp& timestamp) const;

	virtual std::vector<StatisticsMessage> loadAllStatisticsMessages() const;

	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index documentFrequency(
			const std::string& type,
			const std::string& term) const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual Index structTypeNumber( const std::string& structname) const;

	virtual Index termTypeNumber( const std::string& type) const;

	virtual Index termValueNumber( const std::string& value) const;

	virtual bool isForwardIndexTerm( const std::string& type) const;

	virtual ValueIteratorInterface* createTermTypeIterator() const;

	virtual ValueIteratorInterface* createStructTypeIterator() const;

	virtual ValueIteratorInterface* createTermValueIterator() const;

	virtual ValueIteratorInterface* createDocIdIterator() const;

	virtual ValueIteratorInterface* createUserNameIterator() const;

	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const;

	virtual StorageDumpInterface* createDump(
			const std::string& keyprefix) const;

	virtual bool checkStorage( std::ostream& errorlog) const;

	virtual void close();
	virtual void compaction();

	virtual long diskUsage() const;
	virtual BlockStatistics blockStatistics() const;

	virtual const char** getConfigParameters() const;
	virtual std::string config() const;

public:/*Storage (constructor)*/
	/// \param[in] termnomap_source end of line separated list of terms to define first
	void loadTermnoMap( const char* termnomap_source);

public:/*QueryEval,AttributeReader,documentTermIterator,structIterator*/
	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getStructType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getUserno( const std::string& name) const;
	Index getAttributeno( const std::string& name) const;
	std::vector<std::string> getAttributeNames() const;
	Index userId( const std::string& username) const;

public:/*StorageTransaction*/
	const std::string& statisticsPath() const {return m_statisticsPath;}

	void getVariablesWriteBatch(
			DatabaseTransactionInterface* transaction,
			int nof_documents_incr);

	void releaseTransaction( const std::vector<Index>& refreshList);

	void declareNofDocumentsInserted( int incr);
	Index nofAttributeTypes();

	KeyAllocatorInterface* createTypenoAllocator();
	KeyAllocatorInterface* createStructnoAllocator();
	KeyAllocatorInterface* createDocnoAllocator();
	KeyAllocatorInterface* createUsernoAllocator();
	KeyAllocatorInterface* createAttribnoAllocator();
	KeyAllocatorInterface* createTermnoAllocator();

	bool withAcl() const;

	Index allocTermno();
	Index allocDocno();

	Index allocTypenoImm( const std::string& name);		///< immediate allocation of a term type
	Index allocStructnoImm( const std::string& name);	///< immediate allocation of a struct type
	Index allocUsernoImm( const std::string& name);		///< immediate allocation of a user number
	Index allocAttribnoImm( const std::string& name);	///< immediate allocation of a attribute number

	StatisticsBuilderInterface* getStatisticsBuilder();

public:/*StorageTransaction,StorageMetaDataTransaction*/
	friend class TransactionLock;
	class TransactionLock
	{
	public:
		TransactionLock( StorageClient* storage_)
			:m_mutex(&storage_->m_transaction_mutex)
		{
			m_mutex->lock();
		}
		~TransactionLock()
		{
			m_mutex->unlock();
		}

	private:
		strus::mutex* m_mutex;
	};
	strus::shared_ptr<MetaDataBlockCache> getMetaDataBlockCacheRef() const
	{
		return m_metaDataBlockCache;
	}

public:/*StorageMetaDataTransaction*/
	void resetMetaDataBlockCache( const strus::shared_ptr<MetaDataBlockCache>& mdcache);

public:/*StatisticsBuilder*/
	Index documentFrequency( strus::Index typeno, strus::Index termno) const;

public:/*StorageDocumentChecker,AclIterator*/
	IndexSetIterator getAclIterator( strus::Index docno) const;
	IndexSetIterator getUserAclIterator( strus::Index userno) const;

public:/*StatisticsIterator*/
	///\brief Get the document frequency cache
	DocumentFrequencyCache* getDocumentFrequencyCache();

public:/*strusResizeBlocks,StorageDocumentChecker,StorageTransaction*/
	Index maxTermTypeNo() const;
	Index maxStructTypeNo() const;
	DatabaseClientInterface* databaseClient()
	{
		return m_database.get();
	}
	const DatabaseClientInterface* databaseClient() const
	{
		return m_database.get();
	}

private:
	void init( const std::string& databaseConfig);
	void loadVariables( DatabaseClientInterface* database_);
	void storeVariables();
	// \brief Filling document frequency cache
	// \note Neither this method nor the document frequency cache is ever used -- dead code
	void fillDocumentFrequencyCache();

private:
	const DatabaseInterface* m_dbtype;			///< type of key value store database interface
	Reference<DatabaseClientInterface> m_database;		///< reference to key value store database
	char const** m_cfgparam;				///< list of configuration parameters
	strus::AtomicCounter<Index> m_next_typeno;		///< next index to assign to a new term type
	strus::AtomicCounter<Index> m_next_termno;		///< next index to assign to a new term value
	strus::AtomicCounter<Index> m_next_structno;		///< next index to assign to a new structure type
	strus::AtomicCounter<Index> m_next_docno;		///< next index to assign to a new document id
	strus::AtomicCounter<Index> m_next_userno;		///< next index to assign to a new user id
	strus::AtomicCounter<Index> m_next_attribno;		///< next index to assign to a new attribute name
	strus::AtomicCounter<Index> m_nof_documents;		///< number of documents inserted

	strus::mutex m_transaction_mutex;			///< mutual exclusion in the critical part of a transaction
	strus::mutex m_immalloc_typeno_mutex;			///< mutual exclusion in the critical part of immediate allocation of typeno
	strus::mutex m_immalloc_structno_mutex;			///< mutual exclusion in the critical part of immediate allocation of structno
	strus::mutex m_immalloc_attribno_mutex;			///< mutual exclusion in the critical part of immediate allocation of attribno
	strus::mutex m_immalloc_userno_mutex;			///< mutual exclusion in the critical part of immediate allocation of userno

	strus::shared_ptr<MetaDataBlockCache> m_metaDataBlockCache;///< read cache for meta data blocks
	Reference<DocumentFrequencyCache> m_documentFrequencyCache; ///< reference to document frequency cache

	bool m_close_called;					///< true if close was already called
	const StatisticsProcessorInterface* m_statisticsProc;	///< statistics message processor
	std::string m_statisticsPath;				///< storage path for statistics, equals storage path if not explicitely defined differently
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


