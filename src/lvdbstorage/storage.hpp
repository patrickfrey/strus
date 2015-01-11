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
#ifndef _STRUS_LVDB_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_HPP_INCLUDED
#include "strus/storageInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "metaDataBlockCache.hpp"
#include "databaseKey.hpp"
#include "indexSetIterator.hpp"
#include "varSizeNodeTree.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <boost/thread/mutex.hpp>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class DocnoIteratorInterface;
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

/// \brief Strus IR storage implementation based on LevelDB
class Storage
	:public StorageInterface
{
public:
	/// \param[in] path of the storage
	/// \param[in] cachesize_k number of K LRU cache for nodes
	Storage( const std::string& path_, unsigned int cachesize_k, const char* termnomap_source=0);
	virtual ~Storage();

	virtual void close();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid) const;

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type) const;

	virtual DocnoIteratorInterface*
			createInvertedAclIterator(
				const std::string& username) const;
	
	virtual StorageTransactionInterface* createTransaction();

	virtual StorageDocumentInterface* 
			createDocumentChecker(
				const std::string& docid,
				const std::string& logfilename) const;
	
	virtual Index allocDocnoRange( std::size_t nofDocuments);

	virtual MetaDataReaderInterface* createMetaDataReader() const;

	virtual AttributeReaderInterface* createAttributeReader() const;

	virtual Index nofDocumentsInserted() const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	Index userId( const std::string& username) const;

	virtual std::vector<StatCounterValue> getStatistics() const;

public:/*QueryEval*/
	Index getTermValue( const std::string& name) const;
	Index getTermType( const std::string& name) const;
	Index getDocno( const std::string& name) const;
	Index getUserno( const std::string& name) const;
	Index getAttributeName( const std::string& name) const;

public:/*StorageTransaction*/
	void releaseTransaction( const std::vector<Index>& refreshList);

	void declareNofDocumentsInserted( int value);
	Index nofAttributeTypes();

	KeyAllocatorInterface* createTypenoAllocator();
	KeyAllocatorInterface* createDocnoAllocator();
	KeyAllocatorInterface* createUsernoAllocator();
	KeyAllocatorInterface* createAttribnoAllocator();
	KeyAllocatorInterface* createTermnoAllocator();

	bool withAcl() const;

	Index allocTermno();
	Index allocTypenoIm( const std::string& name, bool& isNew);///< immediate allocation of a term type
	Index allocDocnoIm( const std::string& name, bool& isNew); ///< immediate allocation of a doc number
	Index allocUsernoIm( const std::string& name, bool& isNew); ///< immediate allocation of a user number
	Index allocAttribnoIm( const std::string& name, bool& isNew);///< immediate allocation of a attribute number

	friend class TransactionLock;
	class TransactionLock
	{
	public:
		TransactionLock( Storage* storage_)
			:m_mutex(&storage_->m_transaction_mutex)
		{
			m_mutex->lock();
		}
		~TransactionLock()
		{
			m_mutex->unlock();
		}

	private:
		boost::mutex* m_mutex;
	};

public:/*StorageDocumentChecker*/
	IndexSetIterator getAclIterator( const Index& docno) const;
	IndexSetIterator getUserAclIterator( const Index& userno) const;

private:
	void cleanup();
	void loadTermnoMap( const char* termnomap_source);
	Index loadIndexValue(
		const DatabaseKey::KeyPrefix type,
		const std::string& name) const;

	void loadVariables();
	void storeVariables();

private:
	std::string m_path;					///< levelDB storage path 
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB

	Index m_next_typeno;					///< next index to assign to a new term type
	Index m_next_termno;					///< next index to assign to a new term value
	Index m_next_docno;					///< next index to assign to a new document id
	Index m_next_userno;					///< next index to assign to a new user id
	Index m_next_attribno;					///< next index to assign to a new attribute name
	boost::mutex m_mutex_typeno;
	boost::mutex m_mutex_termno;
	boost::mutex m_mutex_docno;
	boost::mutex m_mutex_userno;
	boost::mutex m_mutex_attribno;

	Index allocNameIm(
		DatabaseKey::KeyPrefix prefix,
		Index& counter,
		const std::string& name, bool& isNew);

	Index m_nof_documents;					///< number of documents inserted
	boost::mutex m_nof_documents_mutex;

	Index m_transactionCnt;
	boost::mutex m_transactionCnt_mutex;			///< mutual exclusion for accessing global counters

	boost::mutex m_transaction_mutex;			///< mutual exclusion for the critical part of a transaction

	MetaDataDescription m_metadescr;			///< description of the meta data
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks
	VarSizeNodeTree* m_termno_map;				///< map of the most important (most frequent) terms, if specified
};

}
#endif


