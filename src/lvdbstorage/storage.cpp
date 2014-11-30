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
#include "storageTransaction.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseKey.hpp"
#include "forwardIterator.hpp"
#include "indexPacker.hpp"
#include "blockStorage.hpp"
#include "statistics.hpp"
#include "attributeReader.hpp"
#include "globalKeyMap.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <leveldb/cache.h>

using namespace strus;

Storage::Storage( const std::string& path_, unsigned int cachesize_k)
	:m_path(path_)
	,m_db(0)
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_transactionCnt(0)
	,m_metaDataBlockCache(0)
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
			m_metadescr.load( m_db);
			m_metaDataBlockCache = new MetaDataBlockCache( m_db, m_metadescr);

			loadVariables();
		}
		catch (const std::bad_alloc&)
		{
			if (m_metaDataBlockCache) delete m_metaDataBlockCache; 
			m_metaDataBlockCache = 0;
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

void Storage::releaseTransaction( const std::vector<Index>& refreshList)
{
	// Refresh all entries touched by the inserts/updates written
	std::vector<Index>::const_iterator ri = refreshList.begin(), re = refreshList.end();
	for (; ri != re; ++ri)
	{
		m_metaDataBlockCache->declareVoid( *ri);
	}
	m_metaDataBlockCache->refresh();

	storeVariables();
	boost::mutex::scoped_lock( m_global_counter_mutex);
	if (--m_transactionCnt == 0)
	{
		m_typeno_map.clear();
		m_termno_map.clear();
		m_docno_map.clear();
		m_attribno_map.clear();
	}
}

void Storage::loadVariables()
{
	boost::mutex::scoped_lock( m_global_counter_mutex);
	GlobalKeyMap variableMap( m_db, DatabaseKey::VariablePrefix, 0);
	m_next_termno = variableMap.lookUp( "TermNo");
	m_next_typeno = variableMap.lookUp( "TypeNo");
	m_next_docno = variableMap.lookUp( "DocNo");
	m_next_attribno = variableMap.lookUp( "AttribNo");
	m_nof_documents = variableMap.lookUp( "NofDocs");
}

void Storage::storeVariables()
{
	boost::mutex::scoped_lock( m_global_counter_mutex);
	GlobalKeyMap variableMap( m_db, DatabaseKey::VariablePrefix, 0);
	variableMap.store( "TermNo", m_next_termno);
	variableMap.store( "TypeNo", m_next_typeno);
	variableMap.store( "DocNo", m_next_docno);
	variableMap.store( "AttribNo", m_next_attribno);
	variableMap.store( "NofDocs", m_nof_documents);

	variableMap.getWriteBatch( m_global_counter_batch);
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Write( options, &m_global_counter_batch);
	m_global_counter_batch.Clear();
	if (!status.ok())
	{
		throw std::runtime_error( std::string( "error when writing global counter batch: ") + status.ToString());
	}
}

Index Storage::allocGlobalCounter(
		const std::string& name_,
		Index& next_,
		std::map<std::string,Index> map_,
		bool& isNew_)
{
	std::map<std::string,Index>::iterator ki = map_.lower_bound( name_);
	if (ki == map_.end())
	{
		isNew_ = true;
		map_.insert( ki, std::pair<std::string,Index>( name_, next_));
		return next_++;
	}
	else
	{
		isNew_ = false;
		return ki->second;
	}
}

Index Storage::allocTermno( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock( m_termno_mutex);
	return allocGlobalCounter( name, m_next_termno, m_termno_map, isNew);
}

Index Storage::allocTypeno( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock( m_typeno_mutex);
	return allocGlobalCounter( name, m_next_typeno, m_typeno_map, isNew);
}

Index Storage::allocDocno( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock( m_docno_mutex);
	return allocGlobalCounter( name, m_next_docno, m_docno_map, isNew);
}

Index Storage::allocAttribno( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock( m_attribno_mutex);
	return allocGlobalCounter( name, m_next_attribno, m_attribno_map, isNew);
}

void Storage::close()
{
	storeVariables();

	boost::mutex::scoped_lock( m_global_counter_mutex);
	if (m_transactionCnt)
	{
		throw std::runtime_error("cannot close storage with an alive transactions");
	}
}

Storage::~Storage()
{
	m_global_counter_batch.Clear();
	try
	{
		close();
	}
	catch (...)
	{
		//... silently ignored. Call close directly to catch errors
	}
	delete m_metaDataBlockCache;
	delete m_db;
	if (m_dboptions.block_cache) delete m_dboptions.block_cache;
}

Index Storage::loadIndexValue(
	const DatabaseKey::KeyPrefix type,
	const std::string& name) const
{
	KeyValueStorage termstor( m_db, type, false);
	const KeyValueStorage::Value* val = termstor.load( name);
	if (!val) return 0;
	char const* vi = val->ptr();
	return unpackIndex( vi, vi+val->size());
}

Index Storage::getTermValue( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::TermValuePrefix, name);
}

Index Storage::getTermType( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::TermTypePrefix, name);
}

Index Storage::getDocno( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::DocIdPrefix, name);
}

Index Storage::getAttributeName( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::AttributeKeyPrefix, name);
}

PostingIteratorInterface*
	Storage::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = getTermType( typestr);
	Index termno = getTermValue( termstr);
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

StorageTransactionInterface*
	Storage::createTransaction()
{
	boost::mutex::scoped_lock( m_global_counter_mutex);
	++m_transactionCnt;
	return new StorageTransaction( this, m_db, &m_metadescr);
}

Index Storage::allocDocnoRange( std::size_t nofDocuments)
{
	boost::mutex::scoped_lock( m_docno_mutex);
	Index rt = m_next_docno;
	m_next_docno += nofDocuments;
	return rt;
}

void Storage::declareNofDocumentsInserted( int value)
{
	boost::mutex::scoped_lock( m_global_counter_mutex);
	m_nof_documents += value;
}

Index Storage::nofAttributeTypes()
{
	return m_next_termno -1;
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
	return getDocno( docid);
}

AttributeReaderInterface* Storage::createAttributeReader() const
{
	return new AttributeReader( this, m_db);
}

MetaDataReaderInterface* Storage::createMetaDataReader() const
{
	return new MetaDataReader( m_metaDataBlockCache, &m_metadescr);
}

std::vector<StatCounterValue> Storage::getStatistics() const
{
	std::vector<StatCounterValue> rt;
	Statistics::const_iterator si = Statistics::begin(), se = Statistics::end();
	for (; si != se; ++si)
	{
		rt.push_back( StatCounterValue( si.typeName(), si.value()));
	}
	return rt;
}


