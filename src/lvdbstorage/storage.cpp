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
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "storage.hpp"
#include "storageInserter.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseKey.hpp"
#include "forwardIterator.hpp"
#include "indexPacker.hpp"
#include "metaDataReader.hpp"
#include "docnoBlockReader.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <leveldb/cache.h>

using namespace strus;

Storage::Storage( const std::string& path_, unsigned int cachesize_k)
	:m_path(path_)
	,m_db(0)
	,m_next_termno(0)
	,m_next_typeno(0)
	,m_next_docno(0)
	,m_nof_documents(0)
	,m_dfMap(0)
	,m_metaDataBlockMap(0)
	,m_metaDataBlockCache(0)
	,m_docnoBlockMap(0)
	,m_globalKeyMap(0)
	,m_nofInserterCnt(0)
	,m_flushCnt(0)
{
	// Compression reduces size of index by 25% and has about 10% better performance
	// m_dboptions.compression = leveldb::kNoCompression;
	m_dboptions.create_if_missing = false;
	if (cachesize_k)
	{
		if (cachesize_k * 1024 < cachesize_k) throw std::runtime_error("size of cache out of range");
		m_dboptions.block_cache = leveldb::NewLRUCache( cachesize_k * 1024);
	}
	leveldb::Status status = leveldb::DB::Open( m_dboptions, path_, &m_db);
	if (status.ok())
	{
		try
		{
			m_globalKeyMap = new GlobalKeyMap( m_db);
			m_dfMap = new DocumentFrequencyMap( m_db);
			m_metaDataBlockCache = new MetaDataBlockCache( m_db);
			m_metaDataBlockMap = new MetaDataBlockMap( m_db);
			m_docnoBlockMap = new DocnoBlockMap( m_db);

			m_next_termno = keyLookUp( DatabaseKey::VariablePrefix, "TermNo");
			m_next_typeno = keyLookUp( DatabaseKey::VariablePrefix, "TypeNo");
			m_next_docno = keyLookUp( DatabaseKey::VariablePrefix, "DocNo");
			m_nof_documents = keyLookUp( DatabaseKey::VariablePrefix, "NofDocs");
		}
		catch (const std::bad_alloc&)
		{
			
			if (m_dfMap) delete m_metaDataBlockMap;
			m_dfMap = 0;
			if (m_metaDataBlockMap) delete m_metaDataBlockMap;
			m_metaDataBlockMap = 0;
			if (m_metaDataBlockCache) delete m_metaDataBlockCache; 
			m_metaDataBlockCache = 0;
			if (m_docnoBlockMap) delete m_docnoBlockMap;
			m_docnoBlockMap = 0;
			if (m_globalKeyMap) delete m_globalKeyMap;
			m_globalKeyMap = 0;
			if (m_db) delete m_db;
			m_db = 0;
			if (m_dboptions.block_cache) delete m_dboptions.block_cache;
			m_dboptions.block_cache = 0;
		}
	}
	else
	{
		std::string err = status.ToString();
		if (!!m_dboptions.block_cache)
		{
			if (m_dboptions.block_cache) delete m_dboptions.block_cache;
			m_dboptions.block_cache = 0;
		}
		if (!!m_db)
		{
			delete m_db;
			m_db = 0;
		}
		throw std::runtime_error( std::string( "failed to open storage: ") + err);
	}
}

void Storage::writeInserterBatch()
{
	m_dfMap->getWriteBatch( m_inserter_batch);
	m_metaDataBlockMap->getWriteBatch( m_inserter_batch, *m_metaDataBlockCache);
	m_docnoBlockMap->getWriteBatch( m_inserter_batch);
	m_globalKeyMap->getWriteBatch( m_inserter_batch);

	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Write( options, &m_inserter_batch);
	if (!status.ok())
	{
		throw std::runtime_error( std::string( "error when writing inserter batch: ") + status.ToString());
	}
	m_inserter_batch.Clear();

	// Refresh all entries touched by the inserts/updates written
	m_metaDataBlockCache->refresh();
}

void Storage::close()
{
	boost::mutex::scoped_lock( m_nofInserterCnt_mutex);
	if (m_nofInserterCnt)
	{
		throw std::runtime_error("cannot close storage with an inserter alive");
	}
	if (m_db)
	{
		batchDefineVariable( m_inserter_batch, "TermNo", m_next_termno);
		batchDefineVariable( m_inserter_batch, "TypeNo", m_next_typeno);
		batchDefineVariable( m_inserter_batch, "DocNo", m_next_docno);
		batchDefineVariable( m_inserter_batch, "NofDocs", m_nof_documents);
		writeInserterBatch();
	
		delete m_db;
		m_db = 0;
	}
}

void Storage::checkFlush()
{
	if (++m_flushCnt == NofDocumentsInsertedBeforeAutoCommit)
	{
		flush();
	}
}

void Storage::flush()
{
	writeInserterBatch();
	m_flushCnt = 0;
}

Storage::~Storage()
{
	try
	{
		close();
	}
	catch (...)
	{
		//... silently ignored. Call close directly to catch errors
	}
	delete m_dfMap;
	delete m_metaDataBlockMap;
	delete m_metaDataBlockCache; 
	delete m_docnoBlockMap;
	delete m_globalKeyMap;
	delete m_db;
	if (m_dboptions.block_cache) delete m_dboptions.block_cache;
}

void Storage::writeIndex(const leveldb::Slice& key, const leveldb::Slice& value)
{
	m_inserter_batch.Put( key, value);
}

void Storage::deleteIndex(const leveldb::Slice& key)
{
	m_inserter_batch.Delete( key);
}

void Storage::batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value)
{
	DatabaseKey key( DatabaseKey::VariablePrefix, name);
	std::string valstr;
	packIndex( valstr, value);
	batch.Put( leveldb::Slice( key.ptr(), key.size()),
			leveldb::Slice( valstr.c_str(), valstr.size()));
}

leveldb::Iterator* Storage::newIterator()
{
	if (!m_db) throw std::runtime_error("open read iterator on closed storage");
	return m_db->NewIterator( leveldb::ReadOptions());
}

Index Storage::keyLookUp( DatabaseKey::KeyPrefix prefix, const std::string& keyname) const
{
	return m_globalKeyMap->lookUp( prefix, keyname);
}

Index Storage::keyGetOrCreate( DatabaseKey::KeyPrefix prefix, const std::string& keyname)
{
	Index* counter = 0;
	if (prefix == DatabaseKey::TermTypePrefix)
	{
		counter = &m_next_typeno;
	}
	else if (prefix == DatabaseKey::TermValuePrefix)
	{
		counter = &m_next_termno;
	}
	else if (prefix == DatabaseKey::DocIdPrefix)
	{
		counter = &m_next_docno;
	}
	else
	{
		throw std::logic_error( "internal: unknown prefix for string key of global variable");
	}
	return m_globalKeyMap->getOrCreate( prefix, keyname, *counter);
}

PostingIteratorInterface*
	Storage::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( DatabaseKey::TermTypePrefix, typestr);
	Index termno = keyLookUp( DatabaseKey::TermValuePrefix, termstr);
	if (!typeno || !termno)
	{
		return new NullIterator( typeno, termno, termstr.c_str());
	}
	return new PostingIterator( m_db, typeno, termno, termstr.c_str());
}

ForwardIteratorInterface*
	Storage::createForwardIterator(
		const std::string& type)
{
	return new ForwardIterator( this, m_db, type);
}

StorageInserterInterface*
	Storage::createInserter(
		const std::string& docid)
{
	aquireInserter();
	return new StorageInserter( this, docid);
}

void Storage::aquireInserter()
{
	boost::mutex::scoped_lock( m_nofInserterCnt_mutex);
	if (m_nofInserterCnt) throw std::runtime_error( "only one concurrent inserter allowed on this storage");
	++m_nofInserterCnt;
}

void Storage::releaseInserter()
{
	boost::mutex::scoped_lock( m_nofInserterCnt_mutex);
	--m_nofInserterCnt;
}

void Storage::incrementNofDocumentsInserted()
{
	boost::mutex::scoped_lock( m_nof_documents_mutex);
	++m_nof_documents;
}

Index Storage::nofDocumentsInserted() const
{
	return m_nof_documents;
}

Index Storage::maxDocumentNumber() const
{
	return m_next_docno-1;
}

Index Storage::documentNumber( const std::string& docid) const
{
	return m_globalKeyMap->lookUp( DatabaseKey::DocIdPrefix, docid);
}

std::string Storage::documentAttribute( Index docno, char varname) const
{
	DatabaseKey key( (char)DatabaseKey::DocAttributePrefix, docno, varname);
	leveldb::Slice keyslice( key.ptr(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
	if (status.IsNotFound())
	{
		return std::string();
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	return value;
}


float Storage::documentMetaData( Index docno, char varname) const
{
	return m_metaDataBlockCache->getValue( docno, varname);
}

void Storage::defineMetaData( Index docno, char varname, float value)
{
	m_metaDataBlockMap->defineMetaData( docno, varname, value);
}

void Storage::defineDocnoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, unsigned int ff, float weight)
{
	m_docnoBlockMap->defineDocnoPosting(
		termtype, termvalue, docno, ff, weight);
}

void Storage::deleteDocnoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno)
{
	m_docnoBlockMap->deleteDocnoPosting( termtype, termvalue, docno);
}

void Storage::incrementDf( Index typeno, Index termno)
{
	m_dfMap->increment( typeno, termno);
}

void Storage::decrementDf( Index typeno, Index termno)
{
	m_dfMap->decrement( typeno, termno);
}




