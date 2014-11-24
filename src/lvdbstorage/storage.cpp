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
#include "blockStorage.hpp"
#include "statistics.hpp"
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
	,m_next_termno(0)
	,m_next_typeno(0)
	,m_next_docno(0)
	,m_next_attributeno(0)
	,m_nof_documents(0)
	,m_dfMap(0)
	,m_metaDataBlockMap(0)
	,m_metaDataBlockCache(0)
	,m_docnoBlockMap(0)
	,m_posinfoBlockMap(0)
	,m_termTypeMap(0)
	,m_termValueMap(0)
	,m_docIdMap(0)
	,m_variableMap(0)
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
			m_metadescr.load( m_db);
			m_termTypeMap = new GlobalKeyMap( m_db, DatabaseKey::TermTypePrefix);
			m_termValueMap = new GlobalKeyMap( m_db, DatabaseKey::TermValuePrefix);
			m_docIdMap = new GlobalKeyMap( m_db, DatabaseKey::DocIdPrefix);
			m_variableMap = new GlobalKeyMap( m_db, DatabaseKey::VariablePrefix);
			m_attributeNameMap = new GlobalKeyMap( m_db, DatabaseKey::AttributeKeyPrefix);
			m_dfMap = new DocumentFrequencyMap( m_db);
			m_metaDataBlockCache = new MetaDataBlockCache( m_db, m_metadescr);
			m_metaDataBlockMap = new MetaDataBlockMap( m_db, m_metadescr);
			m_docnoBlockMap = new DocnoBlockMap( m_db);
			m_posinfoBlockMap = new PosinfoBlockMap( m_db);

			m_next_termno = m_variableMap->lookUp( "TermNo");
			m_next_typeno = m_variableMap->lookUp( "TypeNo");
			m_next_docno = m_variableMap->lookUp( "DocNo");
			m_next_attributeno = m_variableMap->lookUp( "AttributeNo");
			m_nof_documents = m_variableMap->lookUp( "NofDocs");
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
			if (m_posinfoBlockMap) delete m_posinfoBlockMap;
			m_posinfoBlockMap = 0;
			if (m_termTypeMap) delete m_termTypeMap;
			m_termTypeMap = 0;
			if (m_termValueMap) delete m_termValueMap;
			m_termValueMap = 0;
			if (m_docIdMap) delete m_docIdMap;
			m_docIdMap = 0;
			if (m_variableMap) delete m_variableMap;
			m_variableMap = 0;
			if (m_attributeNameMap) delete m_attributeNameMap;
			m_attributeNameMap = 0;
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
	m_variableMap->store( "TermNo", m_next_termno);
	m_variableMap->store( "TypeNo", m_next_typeno);
	m_variableMap->store( "DocNo", m_next_docno);
	m_variableMap->store( "AttributeNo", m_next_attributeno);
	m_variableMap->store( "NofDocs", m_nof_documents);

	m_dfMap->getWriteBatch( m_inserter_batch);
	m_metaDataBlockMap->getWriteBatch( m_inserter_batch, *m_metaDataBlockCache);
	m_docnoBlockMap->getWriteBatch( m_inserter_batch);
	m_posinfoBlockMap->getWriteBatch( m_inserter_batch);
	m_termTypeMap->getWriteBatch( m_inserter_batch);
	m_termValueMap->getWriteBatch( m_inserter_batch);
	m_docIdMap->getWriteBatch( m_inserter_batch);
	m_variableMap->getWriteBatch( m_inserter_batch);
	m_attributeNameMap->getWriteBatch( m_inserter_batch);

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
		writeInserterBatch();
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
	delete m_metaDataBlockCache; 

	delete m_dfMap;
	delete m_metaDataBlockMap;
	delete m_docnoBlockMap;
	delete m_posinfoBlockMap;
	delete m_termTypeMap;
	delete m_termValueMap;
	delete m_docIdMap;
	delete m_variableMap;
	delete m_attributeNameMap;

	delete m_db;
	if (m_dboptions.block_cache) delete m_dboptions.block_cache;
}

void Storage::writeKeyValue( const leveldb::Slice& key, const leveldb::Slice& value)
{
	m_inserter_batch.Put( key, value);
}

void Storage::deleteKey(const leveldb::Slice& key)
{
	m_inserter_batch.Delete( key);
}

leveldb::Iterator* Storage::newIterator()
{
	if (!m_db) throw std::runtime_error("open read iterator on closed storage");
	return m_db->NewIterator( leveldb::ReadOptions());
}

Index Storage::getTermValue( const std::string& name) const
{
	return m_termValueMap->lookUp( name);
}

Index Storage::getTermType( const std::string& name) const
{
	return m_termTypeMap->lookUp( name);
}

Index Storage::getDocno( const std::string& name) const
{
	return m_docIdMap->lookUp( name);
}

Index Storage::getAttribute( const std::string& name) const
{
	return m_attributeNameMap->lookUp( name);
}

Index Storage::getOrCreateTermValue( const std::string& name)
{
	return m_termValueMap->getOrCreate( name, m_next_termno);
}

Index Storage::getOrCreateTermType( const std::string& name)
{
	return m_termTypeMap->getOrCreate( name, m_next_typeno);
}

Index Storage::getOrCreateDocno( const std::string& name, bool& isNew)
{
	Index oldCounter = m_next_docno;
	Index rt = m_docIdMap->getOrCreate( name, m_next_docno);
	isNew = (oldCounter < m_next_docno);
	return rt;
}

Index Storage::getOrCreateAttribute( const std::string& name)
{
	return m_attributeNameMap->getOrCreate( name, m_next_attributeno);
}

PostingIteratorInterface*
	Storage::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = m_termTypeMap->lookUp( typestr);
	Index termno = m_termValueMap->lookUp( termstr);
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

void Storage::decrementNofDocumentsInserted()
{
	boost::mutex::scoped_lock( m_nof_documents_mutex);
	--m_nof_documents;
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
	return m_docIdMap->lookUp( docid);
}

std::string Storage::documentAttribute( const Index& docno, char varname) const
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

MetaDataReaderInterface* Storage::createMetaDataReader() const
{
	return new MetaDataReader( m_metaDataBlockCache, &m_metadescr);
}

void Storage::defineMetaData( const Index& docno, const std::string& varname, float value)
{
	m_metaDataBlockMap->defineMetaData( docno, varname, value);
}

void Storage::defineMetaData( const Index& docno, const std::string& varname, int value)
{
	m_metaDataBlockMap->defineMetaData( docno, varname, value);
}

void Storage::defineMetaData( const Index& docno, const std::string& varname, unsigned int value)
{
	m_metaDataBlockMap->defineMetaData( docno, varname, value);
}

void Storage::deleteMetaData( const Index& docno)
{
	m_metaDataBlockMap->deleteMetaData( docno);
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

void Storage::definePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, const std::vector<Index>& posinfo)
{
	m_posinfoBlockMap->definePosinfoPosting(
		termtype, termvalue, docno, posinfo);
}

void Storage::deletePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno)
{
	m_posinfoBlockMap->deletePosinfoPosting( termtype, termvalue, docno);
}


void Storage::incrementDf( const Index& typeno, const Index& termno)
{
	m_dfMap->increment( typeno, termno);
}

void Storage::decrementDf( const Index& typeno, const Index& termno)
{
	m_dfMap->decrement( typeno, termno);
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

void Storage::deleteAttributes( const Index& docno)
{
	leveldb::Iterator* vi = newIterator();
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	DatabaseKey docattribkey( (char)DatabaseKey::DocAttributePrefix, docno);
	for (vi->Seek( leveldb::Slice( docattribkey.ptr(), docattribkey.size()));
		vi->Valid(); vi->Next())
	{
		if (docattribkey.size() > vi->key().size()
		||  0!=std::memcmp( vi->key().data(), (char*)docattribkey.ptr(), docattribkey.size()))
		{
			//... end of document reached
			break;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE ATTRIBUTE [" << vi->key().ToString() << "]" << std::endl;
#endif
		deleteKey( vi->key());
	}
}

void Storage::deleteIndex( const Index& docno)
{
	typedef std::pair<Index,Index> TermMapKey;
	std::set<TermMapKey> oldcontent;

	DatabaseKey invkey( (char)DatabaseKey::ForwardIndexPrefix, docno);
	std::size_t invkeysize = invkey.size();
	leveldb::Slice invkeyslice( invkey.ptr(), invkey.size());

	leveldb::Iterator* vi = newIterator();
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	//[1] Iterate on key prefix elements [ForwardIndexPrefix, docno, typeno, *] and mark them as deleted
	//	Extract typeno and valueno from key [ForwardIndexPrefix, docno, typeno, pos] an mark term as old content (do delete)
	for (vi->Seek( invkeyslice); vi->Valid(); vi->Next())
	{
		if (invkeysize > vi->key().size() || 0!=std::memcmp( vi->key().data(), invkey.ptr(), invkeysize))
		{
			//... end of document reached
			break;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE INV [" << vi->key().ToString() << "]" << std::endl;
#endif
		deleteKey( vi->key());

		const char* ki = vi->key().data() + invkeysize;
		const char* ke = ki + vi->key().size();
		Index typeno = unpackIndex( ki, ke);

		const char* valuestr = vi->value().data();
		std::size_t valuesize = vi->value().size();
		Index valueno = m_termValueMap->lookUp( std::string( valuestr, valuesize));

		oldcontent.insert( TermMapKey( typeno, valueno));
	}

	//[2] Iterate on 'oldcontent' elements built in [1.1] 
	//	and mark delete the postings
	std::set<TermMapKey>::const_iterator di = oldcontent.begin(), de = oldcontent.end();
	for (; di != de; ++di)
	{
		deleteDocnoPosting( di->first, di->second, docno);
		deletePosinfoPosting( di->first, di->second, docno);

#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE TERMS [" << di->first << " " << di->second << " " << docno << "]" << std::endl;
#endif
		decrementDf( di->first, di->second);
	}
}

void Storage::deleteDocument( const std::string& docid)
{
	Index docno = m_docIdMap->lookUp( docid);
	if (docno == 0) return;

	//[1] Delete metadata:
	deleteMetaData( docno);

	//[2] Delete attributes:
	deleteAttributes( docno);
	
	//[3] Delete index elements (forward index and inverted index):
	deleteIndex( docno);

	//[4] Decrement the total number of documents inserted:
	decrementNofDocumentsInserted();

	//[5] Do submit the write to the database:
	checkFlush();
}


