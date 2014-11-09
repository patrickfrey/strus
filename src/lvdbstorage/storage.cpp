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
#include "strus/iteratorInterface.hpp"
#include "strus/forwardIndexViewerInterface.hpp"
#include "storage.hpp"
#include "iterator.hpp"
#include "nullIterator.hpp"
#include "transaction.hpp"
#include "databaseKey.hpp"
#include "forwardIndexViewer.hpp"
#include "indexPacker.hpp"
#include "metaDataReader.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <leveldb/cache.h>

using namespace strus;

Storage::Storage( const std::string& path_, unsigned int cachesize_k)
	:m_path(path_),m_db(0),m_flushCnt(0)
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
		m_next_termno = keyLookUp( DatabaseKey::VariablePrefix, "TermNo");
		m_next_typeno = keyLookUp( DatabaseKey::VariablePrefix, "TypeNo");
		m_next_docno = keyLookUp( DatabaseKey::VariablePrefix, "DocNo");
		m_nof_documents = keyLookUp( DatabaseKey::VariablePrefix, "NofDocs");
		if (m_nof_documents) m_nof_documents -= 1;
	}
	else
	{
		std::string err = status.ToString();
		if (!!m_db)
		{
			delete m_db;
			m_db = 0;
		}
		throw std::runtime_error( std::string( "failed to open storage: ") + err);
	}
}

void Storage::close()
{
	if (m_db)
	{
		flush();

		leveldb::WriteBatch batch;
		batchDefineVariable( batch, "TermNo", m_next_termno);
		batchDefineVariable( batch, "TypeNo", m_next_typeno);
		batchDefineVariable( batch, "DocNo", m_next_docno);
		batchDefineVariable( batch, "NofDocs", m_nof_documents+1);
	
		leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			throw std::runtime_error( std::string("error when flushing and closing storage") + status.ToString());
		}
		batch.Clear();
		delete m_db;
		m_db = 0;
	}
}

void Storage::checkFlush()
{
	if (++m_flushCnt == 1000)
	{
		flush();
		m_flushCnt = 0;
	}
}

void Storage::flush()
{
	flushNewKeys();
	flushDfs();
	flushMetaData();
	flushIndex();
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
		if (m_db) delete m_db;
		if (m_dboptions.block_cache) delete m_dboptions.block_cache;
	}
}

void Storage::writeIndex(const leveldb::Slice& key, const leveldb::Slice& value)
{
	boost::mutex::scoped_lock( m_indexBatch_mutex);
	m_indexBatch.Put( key, value);
}

void Storage::deleteIndex(const leveldb::Slice& key)
{
	boost::mutex::scoped_lock( m_indexBatch_mutex);
	m_indexBatch.Delete( key);
}

void Storage::flushNewKeys()
{
	boost::mutex::scoped_lock( m_mutex);
	if (m_newKeyMap.size() == 0) return;

	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &m_newKeyBatch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	m_newKeyBatch.Clear();
	m_newKeyMap.clear();
	m_nof_documents += 1;
}

void Storage::batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value)
{
	std::string encoded_value;
	packIndex( encoded_value, value);
	batch.Put( Storage::keyString( DatabaseKey::VariablePrefix, name), encoded_value);
}

leveldb::Iterator* Storage::newIterator()
{
	if (!m_db) throw std::runtime_error("open read iterator on closed storage");
	return m_db->NewIterator( leveldb::ReadOptions());
}

std::string Storage::keyString( DatabaseKey::KeyPrefix prefix, const std::string& keyname)
{
	std::string rt;
	rt.push_back( (char)prefix);
	rt.append( keyname);
	return rt;
}

Index Storage::keyLookUp( const std::string& keystr) const
{
	if (!m_db) throw std::runtime_error("read on closed storage");
	std::string value;
	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

Index Storage::keyLookUp( DatabaseKey::KeyPrefix prefix, const std::string& keyname) const
{

	std::string key = keyString( prefix, keyname);
	return keyLookUp( key);
}

Index Storage::keyGetOrCreate( DatabaseKey::KeyPrefix prefix, const std::string& keyname)
{
	if (!m_db) throw std::runtime_error("read on closed storage");
	std::string key = keyString( prefix, keyname);
	leveldb::Slice keyslice( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
	if (status.IsNotFound())
	{
		boost::mutex::scoped_lock( m_mutex);
		NewKeyMap::const_iterator ki = m_newKeyMap.find( key);
		if (ki != m_newKeyMap.end())
		{
			return ki->second;
		}
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
		if (status.IsNotFound())
		{
			Index rt = 0;
			if (prefix == DatabaseKey::TermTypePrefix)
			{
				rt = m_next_typeno++;
			}
			else if (prefix == DatabaseKey::TermValuePrefix)
			{
				rt = m_next_termno++;
			}
			else if (prefix == DatabaseKey::DocIdPrefix)
			{
				rt = m_next_docno++;
			}
			else
			{
				throw std::logic_error( "internal: Cannot create index value");
			}
			std::string valuebuf;
			packIndex( valuebuf, rt);
			m_newKeyMap[ key] = rt;
			m_newKeyBatch.Put( keyslice, valuebuf);
			return rt;
		}
		else if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
	}
	else if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

IteratorInterface*
	Storage::createTermOccurrenceIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( DatabaseKey::TermTypePrefix, typestr);
	Index termno = keyLookUp( DatabaseKey::TermValuePrefix, termstr);
	if (!typeno || !termno)
	{
		return new NullIterator( typeno, termno, termstr.c_str());
	}
	return new Iterator( m_db, typeno, termno, termstr.c_str());
}

ForwardIndexViewerInterface*
	Storage::createForwardIndexViewer(
		const std::string& type)
{
	return new ForwardIndexViewer( this, m_db, type);
}

StorageInterface::TransactionInterface*
	Storage::createTransaction(
		const std::string& docid)
{
	return new Transaction( this, docid);
}

void Storage::incrementNofDocumentsInserted()
{
	boost::mutex::scoped_lock( m_mutex);
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
	return keyLookUp( DatabaseKey::DocIdPrefix, docid);
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

void Storage::defineMetaData( Index docno, char varname, float value)
{
	Index blockno = MetaDataBlock::blockno( docno);
	MetaDataKey key( varname, blockno);
	boost::mutex::scoped_lock( m_mutex_metaDataBlockMap);
	MetaDataBlockMap::const_iterator mi = m_metaDataBlockMap.find( key);
	if (mi == m_metaDataBlockMap.end())
	{
		MetaDataBlockReference& block = m_metaDataBlockMap[ key];
		block.reset( new MetaDataBlock( m_db, blockno, varname));
		block->setValue( docno, value);
	}
	else
	{
		mi->second->setValue( docno, value);
	}
}

void Storage::flushIndex()
{
	boost::mutex::scoped_lock( m_indexBatch_mutex);
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &m_indexBatch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	m_indexBatch.Clear();
}

void Storage::flushMetaData()
{
	if (!m_db)
	{
		throw std::runtime_error( "no storage defined (flush metadata)");
	}
	leveldb::WriteBatch batch;
	boost::mutex::scoped_lock( m_mutex_metaDataBlockMap);
	MetaDataBlockMap::const_iterator
		mi = m_metaDataBlockMap.begin(),
		me = m_metaDataBlockMap.end();
	for (; mi != me; ++mi)
	{
		mi->second->addToBatch( batch);
	}
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	batch.Clear();
	m_metaDataBlockMap.clear();
}

void Storage::incrementDf( Index typeno, Index termno)
{
	boost::mutex::scoped_lock( m_mutex);
	DfKey key( typeno, termno);
	m_dfMap[ key] += 1;
}

void Storage::decrementDf( Index typeno, Index termno)
{
	boost::mutex::scoped_lock( m_mutex);
	std::pair<Index,Index> key( typeno, termno);
	m_dfMap[ key] -= 1;
}

void Storage::flushDfs()
{
	if (!m_db)
	{
		throw std::runtime_error( "no storage defined (flush dfs)");
	}
	boost::mutex::scoped_lock( m_mutex);
	std::map<DfKey,Index>::const_iterator mi = m_dfMap.begin(), me = m_dfMap.end();
	leveldb::WriteBatch batch;

	for (; mi != me; ++mi)
	{
		std::string keystr;
		keystr.push_back( DatabaseKey::DocFrequencyPrefix);
		packIndex( keystr, mi->first.first);	// ... [typeno]
		packIndex( keystr, mi->first.second);	// ... [valueno]

		leveldb::Slice keyslice( keystr.c_str(), keystr.size());
		Index df;
		std::string value;
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
		if (status.IsNotFound() || value.empty())
		{
			df = mi->second;
		}
		else
		{
			if (!status.ok())
			{
				throw std::runtime_error( status.ToString());
			}
			char const* cc = value.c_str();
			char const* ee = value.c_str() + value.size();
			df = mi->second + unpackIndex( cc, ee);
		}
		enum {MaxValueSize = sizeof(Index)*4};
		char valuebuf[ MaxValueSize];
		std::size_t valuepos = 0;
		packIndex( valuebuf, valuepos, MaxValueSize, df);
		leveldb::Slice valueslice( valuebuf, valuepos);

		batch.Put( keyslice, valueslice);
	}
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	batch.Clear();
	m_dfMap.clear();
}

MetaDataReaderInterface* Storage::createMetaDataReader( char varname) const
{
	return new MetaDataReader( m_db, varname);
}


