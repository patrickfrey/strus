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
#include "strus/index.hpp"
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "private/utils.hpp"
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
	/// \param[in] databaseConfig configuration string (not a filename!) of the database interface to create for this storage
	/// \param[in] termnomap_source end of line separated list of terms to cache for eventually faster lookup
	/// \param[in] statisticsProc_ statistics message processor interface
	/// \param[in] errorhnd_ error buffering interface for error handling
	StorageClient(
			const DatabaseInterface* database_,
			const std::string& databaseConfig,
			const char* termnomap_source,
			const StatisticsProcessorInterface* statisticsProc_,
			ErrorBufferInterface* errorhnd_);
	virtual ~StorageClient();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid) const;

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

	virtual StorageTransactionInterface*
			createTransaction();

	virtual StorageDocumentInterface*
			createDocumentChecker(
				const std::string& docid,
				const std::string& logfilename) const;

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual MetaDataRestrictionInterface* createMetaDataRestriction() const;
	
	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual StatisticsIteratorInterface* createInitStatisticsIterator( bool sign);

	virtual StatisticsIteratorInterface* createUpdateStatisticsIterator();

	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index documentFrequency(
			const std::string& type,
			const std::string& term) const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual ValueIteratorInterface* createTermTypeIterator() const;

	virtual ValueIteratorInterface* createTermValueIterator() const;

	virtual ValueIteratorInterface* createDocIdIterator() const;

	virtual ValueIteratorInterface* createUserNameIterator() const;

	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const;

	virtual bool checkStorage( std::ostream& errorlog) const;

	virtual StorageDumpInterface* createDump( const std::string& keyprefix) const;

public:/*QueryEval,AttributeReader,documentTermIterator*/
	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getUserno( const std::string& name) const;
	Index getAttributeno( const std::string& name) const;
	std::vector<std::string> getAttributeNames() const;
	Index userId( const std::string& username) const;

public:/*StorageTransaction*/
	void getVariablesWriteBatch(
			DatabaseTransactionInterface* transaction,
			int nof_documents_incr);

	void releaseTransaction( const std::vector<Index>& refreshList);

	void declareNofDocumentsInserted( int incr);
	Index nofAttributeTypes();

	KeyAllocatorInterface* createTypenoAllocator();
	KeyAllocatorInterface* createDocnoAllocator();
	KeyAllocatorInterface* createUsernoAllocator();
	KeyAllocatorInterface* createAttribnoAllocator();
	KeyAllocatorInterface* createTermnoAllocator();

	bool withAcl() const;

	Index allocTermno();
	Index allocDocno();

	Index allocTypenoImm( const std::string& name);		///< immediate allocation of a term type
	Index allocUsernoImm( const std::string& name);		///< immediate allocation of a user number
	Index allocAttribnoImm( const std::string& name);	///< immediate allocation of a attribute number

	StatisticsBuilderInterface* getStatisticsBuilder();

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
		utils::Mutex* m_mutex;
	};

public:/*StatisticsBuilder*/
	Index documentFrequency( const Index& typeno, const Index& termno) const;

public:/*StorageDocumentChecker*/
	IndexSetIterator getAclIterator( const Index& docno) const;
	IndexSetIterator getUserAclIterator( const Index& userno) const;

public:/*StatisticsIterator*/
	///\brief Get the document frequency cache
	DocumentFrequencyCache* getDocumentFrequencyCache();
	///\brief Fetch a message from a storage update transaction
	bool fetchNextStatisticsMessage( const char*& msg, std::size_t& msgsize);

public:/*strusResizeBlocks*/
	Index maxTermTypeNo() const;
	DatabaseClientInterface* databaseClient()
	{
		return m_database.get();
	}

private:
	void cleanup();
	void loadTermnoMap( const char* termnomap_source);
	void loadVariables( DatabaseClientInterface* database_);
	void storeVariables();
	void fillDocumentFrequencyCache();

private:
	Reference<DatabaseClientInterface> m_database;		///< reference to key value store database
	utils::AtomicCounter<Index> m_next_typeno;		///< next index to assign to a new term type
	utils::AtomicCounter<Index> m_next_termno;		///< next index to assign to a new term value
	utils::AtomicCounter<Index> m_next_docno;		///< next index to assign to a new document id
	utils::AtomicCounter<Index> m_next_userno;		///< next index to assign to a new user id
	utils::AtomicCounter<Index> m_next_attribno;		///< next index to assign to a new attribute name

	utils::AtomicCounter<Index> m_nof_documents;		///< number of documents inserted

	utils::Mutex m_transaction_mutex;			///< mutual exclusion in the critical part of a transaction
	utils::Mutex m_immalloc_typeno_mutex;			///< mutual exclusion in the critical part of immediate allocation of typeno s
	utils::Mutex m_immalloc_attribno_mutex;			///< mutual exclusion in the critical part of immediate allocation of attribno s
	utils::Mutex m_immalloc_userno_mutex;			///< mutual exclusion in the critical part of immediate allocation of userno s

	MetaDataDescription m_metadescr;			///< description of the meta data
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks

	const StatisticsProcessorInterface* m_statisticsProc;	///< statistics message processor
	Reference<StatisticsBuilderInterface> m_statisticsBuilder; ///< builder of statistics messages from updates by transactions
	Reference<DocumentFrequencyCache> m_documentFrequencyCache; ///< reference to document frequency cache

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


