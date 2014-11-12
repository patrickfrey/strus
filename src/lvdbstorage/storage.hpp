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
	virtual void close();
	virtual ~Storage();

	virtual PostingIteratorInterface*
			createTermPostingIterator(
				const std::string& termtype,
				const std::string& termid);

	virtual ForwardIteratorInterface*
			createForwardIterator(
				const std::string& type);

	virtual StorageInserterInterface*
			createInserter( const std::string& docid);

	virtual MetaDataReaderInterface*
			createMetaDataReader( char varname) const;

	virtual Index nofDocumentsInserted() const;

	virtual Index maxDocumentNumber() const;

	virtual Index documentNumber( const std::string& docid) const;

	virtual std::string documentAttribute( Index docno, char varname) const;

	virtual void incrementDf( Index typeno, Index termno);
	virtual void decrementDf( Index typeno, Index termno);

public:
	void defineMetaData( Index docno, char varname, float value);

	void writeIndex( const leveldb::Slice& key, const leveldb::Slice& value);
	void deleteIndex( const leveldb::Slice& key);

	void incrementNofDocumentsInserted();

	leveldb::Iterator* newIterator();

	static std::string keyString( DatabaseKey::KeyPrefix prefix, const std::string& keyname);
	Index keyLookUp( DatabaseKey::KeyPrefix prefix, const std::string& keyname) const;
	Index keyGetOrCreate( DatabaseKey::KeyPrefix prefix, const std::string& keyname);
	Index keyLookUp( const std::string& keystr) const;

	void checkFlush();
	void flush();

private:
	void batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value);

	void flushNewKeys();
	void flushDfs();
	void flushMetaData();
	void flushIndex();

	struct stlSliceComparator
	{
		bool operator()(const leveldb::Slice& a, const leveldb::Slice& b) const
		{
			return a.compare(b) < 0;
		}
	};
	typedef std::map<std::string,Index,stlSliceComparator> NewKeyMap;

	typedef std::pair<Index,Index> DfKey;
	typedef std::map<DfKey, Index> DfMap;

private:
	std::string m_path;					///< levelDB storage path 
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::Options m_dboptions;				///< options for levelDB
	Index m_next_termno;					///< next index to assign to a new term value
	Index m_next_typeno;					///< next index to assign to a new term type
	Index m_next_docno;					///< next index to assign to a new document id
	Index m_nof_documents;					///< number of documents inserted
	boost::mutex m_mutex;					///< mutex for mutual exclusion for the access of counters (m_next_..) and for the access of keys not defined during a running insertion procedure
	leveldb::WriteBatch m_indexBatch;			///< batch for key/values written by inserters
	boost::mutex m_indexBatch_mutex;			///< mutex for mutual exclusion for writing to the index
	leveldb::WriteBatch m_newKeyBatch;			///< batch for new keys defined.
	NewKeyMap m_newKeyMap;					///< temporary map for the new keys defined
	DfMap m_dfMap;						///< temporary map for the document frequency of new inserted features
	MetaDataBlockMap* m_metaDataBlockMap;			///< map of meta data blocks for writing
	Index m_flushCnt;
};

}
#endif


