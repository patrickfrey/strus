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
#include "databaseKey.hpp"
#include "metaDataBlock.hpp"
#include "metaDataBlockMap.hpp"
#include "metaDataBlockCache.hpp"
#include "docnoBlock.hpp"
#include "docnoBlockMap.hpp"
#include "posinfoBlock.hpp"
#include "posinfoBlockMap.hpp"
#include "documentFrequencyMap.hpp"
#include "globalKeyMap.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class StorageInserterInterface;

/// \brief Strus IR storage implementation based on LevelDB
class Storage
	:public StorageInterface
{
public:
	/// \param[in] path of the storage
	/// \param[in] cachesize_k number of K LRU cache for nodes
	Storage( const std::string& path_, unsigned int cachesize_k);
	virtual ~Storage();

	virtual void close();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid);

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type);

	virtual StorageInserterInterface*
			createInserter( const std::string& docid);

	virtual float documentMetaData( Index docno, char varname) const;

	virtual Index nofDocumentsInserted() const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual std::string documentAttribute( Index docno, char varname) const;

public:
	void incrementDf( Index typeno, Index termno);
	void decrementDf( Index typeno, Index termno);

	enum {NofDocumentsInsertedBeforeAutoCommit=4096};

	void defineMetaData( Index docno, char varname, float value);

	void defineDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, unsigned int ff, float weight);

	void deleteDocnoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno);

	void definePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, const std::vector<Index>& posinfo);

	void deletePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno);

	void writeIndex( const leveldb::Slice& key, const leveldb::Slice& value);
	void deleteIndex( const leveldb::Slice& key);

	void incrementNofDocumentsInserted();

	leveldb::Iterator* newIterator();

	Index keyLookUp( DatabaseKey::KeyPrefix prefix, const std::string& keyname) const;
	Index keyGetOrCreate( DatabaseKey::KeyPrefix prefix, const std::string& keyname);

	void checkFlush();
	void flush();
	void releaseInserter();

private:
	void writeInserterBatch();
	void aquireInserter();

	void batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value);

private:
	std::string m_path;					///< levelDB storage path 
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB

	Index m_next_termno;					///< next index to assign to a new term value
	Index m_next_typeno;					///< next index to assign to a new term type
	Index m_next_docno;					///< next index to assign to a new document id
	Index m_nof_documents;					///< number of documents inserted
	boost::mutex m_nof_documents_mutex;			///< mutual exclusion for accessing m_nof_documents

	leveldb::WriteBatch m_inserter_batch;			///< batch used for an insert chunk written to disk with 'flush()', resp. 'writeInserterBatch()'

	DocumentFrequencyMap* m_dfMap;				///< temporary map for the document frequency of new inserted features
	MetaDataBlockMap* m_metaDataBlockMap;			///< map of meta data blocks for writing
	MetaDataBlockCache* m_metaDataBlockCache;		///< read cache for meta data blocks
	DocnoBlockMap* m_docnoBlockMap;				///< map of docno postings for writing
	PosinfoBlockMap* m_posinfoBlockMap;			///< map of posinfo postings for writing
	GlobalKeyMap* m_globalKeyMap;				///< map of globals in the storage (term numbers, document numbers, etc.)

	boost::mutex m_nofInserterCnt_mutex;			///< mutual exclusion for aquiring inserter
	unsigned int m_nofInserterCnt;				///< counter of inserters
	Index m_flushCnt;					///< counter to do a commit after some inserts
};

}
#endif


