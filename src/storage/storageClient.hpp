/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_STORAGE_HPP_INCLUDED
#define _STRUS_STORAGE_HPP_INCLUDED
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/reference.hpp"
#include "private/utils.hpp"
#include "metaDataBlockCache.hpp"
#include "indexSetIterator.hpp"
#include "compactNodeTrie.hpp"
#include "strus/peerMessageProcessorInterface.hpp"
namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
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
class KeyAllocatorInterface;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DocumentFrequencyCache;
/// \brief Forward declaration
class DocnoRangeAllocatorInterface;
/// \brief Forward declaration
class StorageDumpInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Implementation of the StorageClientInterface
class StorageClient
	:public StorageClientInterface
{
public:
	/// \param[in] database key value store database used by this storage (ownership passed to this)
	/// \param[in] termnomap_source end of line separated list of terms to cache for eventually faster lookup
	/// \param[in] peerMessageProc_ peer message processor interface
	/// \param[in] errorhnd_ error buffering interface for error handling
	StorageClient(
			DatabaseClientInterface* database_,
			const char* termnomap_source,
			const PeerMessageProcessorInterface* peerMessageProc_,
			ErrorBufferInterface* errorhnd_);
	virtual ~StorageClient();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid) const;

	virtual ForwardIteratorInterface*
			createForwardIterator(
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

	virtual DocnoRangeAllocatorInterface* createDocnoRangeAllocator();

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual PeerMessageIteratorInterface* createInitPeerMessageIterator( bool sign);

	virtual PeerMessageIteratorInterface* createUpdatePeerMessageIterator();

	virtual PeerStorageTransactionInterface* createPeerStorageTransaction();

	virtual const PeerMessageProcessorInterface* getPeerMessageProcessor() const;

	virtual GlobalCounter globalNofDocumentsInserted() const;

	virtual Index localNofDocumentsInserted() const;

	virtual GlobalCounter globalDocumentFrequency(
			const std::string& type,
			const std::string& term) const;
	virtual Index localDocumentFrequency(
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

	virtual StorageDumpInterface* createDump() const;

public:/*QueryEval,AttributeReader*/
	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getUserno( const std::string& name) const;
	Index getAttributeno( const std::string& name) const;
	std::vector<std::string> getAttributeNames() const;
	GlobalCounter documentFrequency( const Index& typeno, const Index& termno) const;
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
	Index allocTypenoImm( const std::string& name, bool& isNew);///< immediate allocation of a term type
	Index allocDocnoImm( const std::string& name, bool& isNew); ///< immediate allocation of a doc number
	Index allocUsernoImm( const std::string& name, bool& isNew); ///< immediate allocation of a user number
	Index allocAttribnoImm( const std::string& name, bool& isNew);///< immediate allocation of a attribute number

	Index allocDocnoRange( std::size_t nofDocuments);
	bool deallocDocnoRange( const Index& docno, const Index& size);
	PeerMessageBuilderInterface* getPeerMessageBuilder();

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

public:/*PeerMessageTransaction*/
	void declareGlobalNofDocumentsInserted( const GlobalCounter& increment);
	Index localDocumentFrequency( const Index& typeno, const Index& termno) const;

public:/*StorageDocumentChecker*/
	IndexSetIterator getAclIterator( const Index& docno) const;
	IndexSetIterator getUserAclIterator( const Index& userno) const;

public:/*PeerMessageIterator*/
	///\brief Get the document frequency cache
	DocumentFrequencyCache* getDocumentFrequencyCache();

public:/*PeerMessageIterator*/
	///\brief Fetch a message from a storage update transaction
	bool fetchPeerUpdateMessage( const char*& msg, std::size_t& msgsize);

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
	utils::AtomicCounter<GlobalCounter> m_global_nof_documents; ///< global number of documents inserted

	utils::Mutex m_transaction_mutex;			///< mutual exclusion in the critical part of a transaction

	MetaDataDescription m_metadescr;			///< description of the meta data
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks
	conotrie::CompactNodeTrie* m_termno_map;		///< map of the most important (most frequent) terms, if specified

	const PeerMessageProcessorInterface* m_peerMessageProc;	///< peer message processor
	Reference<PeerMessageBuilderInterface> m_peerMessageBuilder; ///< builder of peer messages from updates by transactions
	Reference<DocumentFrequencyCache> m_documentFrequencyCache; ///< reference to document frequency cache

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}
#endif


