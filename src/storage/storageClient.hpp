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
#include "strus/base/thread.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/atomic.hpp"
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
			const std::string& termid,
			const Index& length) const;

	virtual PostingIteratorInterface*
		createBrowsePostingIterator(
			const MetaDataRestrictionInterface* restriction,
			const Index& maxpos) const;

	virtual PostingIteratorInterface*
		createFieldPostingIterator(
			const std::string& meta_fieldStart,
			const std::string& meta_fieldEnd) const;

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

	virtual StatisticsIteratorInterface* createAllStatisticsIterator( bool sign);

	virtual StatisticsIteratorInterface* createChangeStatisticsIterator();

	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index documentFrequency(
			const std::string& type,
			const std::string& term) const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual Index termTypeNumber( const std::string& type) const;

	virtual bool isForwardIndexTerm( const std::string& type) const;

	virtual ValueIteratorInterface* createTermTypeIterator() const;

	virtual ValueIteratorInterface* createTermValueIterator() const;

	virtual ValueIteratorInterface* createDocIdIterator() const;

	virtual ValueIteratorInterface* createUserNameIterator() const;

	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const;

	virtual bool checkStorage( std::ostream& errorlog) const;

	virtual void close();
	virtual std::string config() const;

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
	Index allocStructnoImm( const std::string& name);	///< immediate allocation of a struct type
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
		strus::mutex* m_mutex;
	};

public:/*StatisticsBuilder*/
	Index documentFrequency( const Index& typeno, const Index& termno) const;

public:/*StorageDocumentChecker,AclIterator*/
	IndexSetIterator getAclIterator( const Index& docno) const;
	IndexSetIterator getUserAclIterator( const Index& userno) const;

public:/*StatisticsIterator*/
	///\brief Get the document frequency cache
	DocumentFrequencyCache* getDocumentFrequencyCache();
	///\brief Fetch a message from a storage update transaction
	bool fetchNextStatisticsMessage( const void*& msg, std::size_t& msgsize);

public:/*strusResizeBlocks*/
	Index maxTermTypeNo() const;
	Index maxStructTypeNo() const;
	DatabaseClientInterface* databaseClient()
	{
		return m_database.get();
	}

private:
	void cleanup();
	void loadTermnoMap( const char* termnomap_source);
	void loadVariables( DatabaseClientInterface* database_);
	void storeVariables();
	// \brief Filling document frequency cache
	// \note Neither this method nor the document frequency cache is ever used -- dead code
	void fillDocumentFrequencyCache();

private:
	Reference<DatabaseClientInterface> m_database;		///< reference to key value store database
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

	MetaDataDescription m_metadescr;			///< description of the meta data
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks

	const StatisticsProcessorInterface* m_statisticsProc;	///< statistics message processor
	Reference<StatisticsBuilderInterface> m_statisticsBuilder; ///< builder of statistics messages from updates by transactions
	Reference<DocumentFrequencyCache> m_documentFrequencyCache; ///< reference to document frequency cache
	bool m_close_called;					///< true if close was already called

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


