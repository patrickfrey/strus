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
#include "strus/docnoIteratorInterface.hpp"
#include "storage.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseKey.hpp"
#include "forwardIterator.hpp"
#include "indexPacker.hpp"
#include "blockStorage.hpp"
#include "statistics.hpp"
#include "attributeReader.hpp"
#include "keyMap.hpp"
#include "keyAllocatorInterface.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>
#include <leveldb/cache.h>

using namespace strus;

enum {ExpectedTransactionSize = (1<<14)};

void Storage::cleanup()
{
	if (m_metaDataBlockCache)
	{
		delete m_metaDataBlockCache; 
		m_metaDataBlockCache = 0;
	}
	if (m_db)
	{
		delete m_db;
		m_db = 0;
	}
	if (m_dboptions.block_cache)
	{
		delete m_dboptions.block_cache;
		m_dboptions.block_cache = 0;
	}
	if (m_termno_map)
	{
		delete m_termno_map;
		m_termno_map = 0;
	}
}

Storage::Storage( const std::string& path_, unsigned int cachesize_k, const char* termnomap_source)
	:m_path(path_)
	,m_db(0)
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_transactionCnt(0)
	,m_metaDataBlockCache(0)
	,m_termno_map(0)
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
			if (termnomap_source) loadTermnoMap( termnomap_source);
		}
		catch (const std::bad_alloc& err)
		{
			cleanup();
			throw err;
		}
		catch (const std::runtime_error& err)
		{
			cleanup();
			throw err;
		}
	}
	else
	{
		std::string err = status.ToString();
		cleanup();
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

	boost::mutex::scoped_lock lock( m_transactionCnt_mutex);
	--m_transactionCnt;
}

void Storage::loadVariables()
{
	KeyMap variableMap( m_db, DatabaseKey::VariablePrefix, 0);
	m_next_termno = variableMap.lookUp( "TermNo");
	m_next_typeno = variableMap.lookUp( "TypeNo");
	m_next_docno = variableMap.lookUp( "DocNo");
	m_next_userno = variableMap.lookUp( "UserNo");
	m_next_attribno = variableMap.lookUp( "AttribNo");
	m_nof_documents = variableMap.lookUp( "NofDocs");
}

void Storage::storeVariables()
{
	leveldb::WriteBatch batch;
	KeyValueStorage varstor( m_db, DatabaseKey::VariablePrefix, false);
	{
		std::string termnoval;
		packIndex( termnoval, m_next_termno);
		varstor.store( "TermNo", termnoval, batch);
	}{
		std::string typenoval;
		packIndex( typenoval, m_next_typeno);
		varstor.store( "TypeNo", typenoval, batch);
	}{
		std::string docnoval;
		packIndex( docnoval, m_next_docno);
		varstor.store( "DocNo", docnoval, batch);
	}
	if (withAcl())
	{
		std::string usernoval;
		packIndex( usernoval, m_next_userno);
		varstor.store( "UserNo", usernoval, batch);
	}{
		std::string attribnoval;
		packIndex( attribnoval, m_next_attribno);
		varstor.store( "AttribNo", attribnoval, batch);
	}{
		std::string nofdocsval;
		packIndex( nofdocsval, m_nof_documents);
		varstor.store( "NofDocs", nofdocsval, batch);
	}
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Write( options, &batch);
	batch.Clear();
	if (!status.ok())
	{
		throw std::runtime_error( std::string( "error when writing global counter batch: ") + status.ToString());
	}
}

void Storage::close()
{
	storeVariables();

	boost::mutex::scoped_lock lock( m_transactionCnt_mutex);
	if (m_transactionCnt)
	{
		throw std::runtime_error("cannot close storage with transactions alive");
	}
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
	cleanup();
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
	if (m_termno_map)
	{
		VarSizeNodeTree::NodeData cached_termno;
		if (m_termno_map->find( name.c_str(), cached_termno)) return cached_termno;
	}
	return loadIndexValue( DatabaseKey::TermValuePrefix, name);
}

Index Storage::getTermType( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::TermTypePrefix, boost::algorithm::to_lower_copy( name));
}

Index Storage::getDocno( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::DocIdPrefix, name);
}

Index Storage::getUserno( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::UserNamePrefix, name);
}

Index Storage::getAttributeName( const std::string& name) const
{
	return loadIndexValue( DatabaseKey::AttributeKeyPrefix, boost::algorithm::to_lower_copy( name));
}

PostingIteratorInterface*
	Storage::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr) const
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
		const std::string& type) const
{
	return new ForwardIterator( this, m_db, type);
}

class InvertedAclIterator
	:public DocnoIteratorInterface
	,public IndexSetIterator
	
{
public:
	InvertedAclIterator( leveldb::DB* db_, const Index& userno_)
		:IndexSetIterator( db_, DatabaseKey::UserAclBlockPrefix, userno_){}
	InvertedAclIterator( const InvertedAclIterator& o)
		:IndexSetIterator(o){}

	virtual Index skipDoc( const Index& docno_)
	{
		return skip(docno_);
	}
	virtual DocnoIteratorInterface* copy() const
	{
		return new InvertedAclIterator(*this);
	}
};

class UnknownUserInvertedAclIterator
	:public DocnoIteratorInterface
{
public:
	UnknownUserInvertedAclIterator(){}
	virtual Index skipDoc( const Index&)	{return 0;}

	virtual DocnoIteratorInterface* copy() const
	{
		return new UnknownUserInvertedAclIterator();
	}
};

DocnoIteratorInterface*
	Storage::createInvertedAclIterator(
		const std::string& username) const
{
	if (!withAcl())
	{
		return 0;
	}
	Index userno = getUserno( username);
	if (userno == 0)
	{
		return new UnknownUserInvertedAclIterator();
	}
	else
	{
		return new InvertedAclIterator( m_db, userno);
	}
}


StorageTransactionInterface*
	Storage::createTransaction()
{
	{
		boost::mutex::scoped_lock lock( m_transactionCnt_mutex);
		++m_transactionCnt;
	}
	return new StorageTransaction( this, m_db, &m_metadescr, m_termno_map);
}

StorageDocumentInterface* 
	Storage::createDocumentChecker(
		const std::string& docid,
		const std::string& logfilename) const
{
	return new StorageDocumentChecker( this, docid, logfilename);
}

Index Storage::allocDocnoRange( std::size_t nofDocuments)
{
	boost::mutex::scoped_lock lock( m_mutex_docno);
	Index rt = m_next_docno;
	m_next_docno += nofDocuments;
	if (m_next_docno <= rt) throw std::runtime_error( "docno allocation error");
	return rt;
}

void Storage::declareNofDocumentsInserted( int value)
{
	boost::mutex::scoped_lock lock( m_nof_documents_mutex);
	m_nof_documents += value;
}

class TypenoAllocator
	:public KeyAllocatorInterface
{
public:
	TypenoAllocator( Storage* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocTypenoIm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use typeno allocator for non immediate alloc");
	}

private:
	Storage* m_storage;
};

class DocnoAllocator
	:public KeyAllocatorInterface
{
public:
	DocnoAllocator( Storage* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocDocnoIm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use docno allocator for non immediate alloc");
	}
private:
	Storage* m_storage;
};

class UsernoAllocator
	:public KeyAllocatorInterface
{
public:
	UsernoAllocator( Storage* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		if (!m_storage->withAcl())
		{
			throw std::runtime_error( "storage configured without ACL. No users can be created");
		}
		return m_storage->allocUsernoIm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use docno allocator for non immediate alloc");
	}
private:
	Storage* m_storage;
};

class AttribnoAllocator
	:public KeyAllocatorInterface
{
public:
	AttribnoAllocator( Storage* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocAttribnoIm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use attribno allocator for non immediate alloc");
	}
private:
	Storage* m_storage;
};

class TermnoAllocator
	:public KeyAllocatorInterface
{
public:
	TermnoAllocator( Storage* storage_)
		:KeyAllocatorInterface(false),m_storage(storage_){}

	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		throw std::logic_error("cannot use termno allocator for immediate alloc");
	}
	virtual Index alloc()
	{
		return m_storage->allocTermno();
	}
private:
	Storage* m_storage;
};


KeyAllocatorInterface* Storage::createTypenoAllocator()
{
	return new TypenoAllocator( this);
}

KeyAllocatorInterface* Storage::createDocnoAllocator()
{
	return new DocnoAllocator( this);
}

KeyAllocatorInterface* Storage::createUsernoAllocator()
{
	return new UsernoAllocator( this);
}

KeyAllocatorInterface* Storage::createAttribnoAllocator()
{
	return new AttribnoAllocator( this);
}

KeyAllocatorInterface* Storage::createTermnoAllocator()
{
	return new TermnoAllocator( this);
}

bool Storage::withAcl() const
{
	return m_next_userno != 0;
}

Index Storage::allocTermno()
{
	boost::mutex::scoped_lock lock( m_mutex_termno);
	return m_next_termno++;
}

Index Storage::allocTypenoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_typeno);
	return allocNameIm( DatabaseKey::TermTypePrefix, m_next_typeno, name, isNew);
}

Index Storage::allocDocnoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_docno);
	return allocNameIm( DatabaseKey::DocIdPrefix, m_next_docno, name, isNew);
}

Index Storage::allocUsernoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_userno);
	return allocNameIm( DatabaseKey::UserNamePrefix, m_next_userno, name, isNew);
}

Index Storage::allocAttribnoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_attribno);
	return allocNameIm( DatabaseKey::AttributeKeyPrefix, m_next_attribno, name, isNew);
}

Index Storage::allocNameIm(
	DatabaseKey::KeyPrefix prefix,
	Index& counter,
	const std::string& name,
	bool& isNew)
{
	Index rt;
	KeyValueStorage stor( m_db, prefix, true);
	const KeyValueStorage::Value* val = stor.load( name);
	if (!val)
	{
		std::string indexval;
		packIndex( indexval, rt = counter++);
		stor.storeIm( name, indexval);
		isNew = true;
	}
	else
	{
		char const* vi = val->ptr();
		rt = unpackIndex( vi, vi+val->size());
		isNew = false;
	}
	return rt;
}

IndexSetIterator Storage::getAclIterator( const Index& docno) const
{
	return IndexSetIterator( m_db, DatabaseKey::AclBlockPrefix, docno);
}

IndexSetIterator Storage::getUserAclIterator( const Index& userno) const
{
	return IndexSetIterator( m_db, DatabaseKey::UserAclBlockPrefix, userno);
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
	Index rt = getDocno( docid);
	if (!rt) throw std::runtime_error( std::string( "document with id '") + docid + "' is not defined in index");
	return rt;
}

Index Storage::userId( const std::string& username) const
{
	Index rt = getUserno( username);
	if (!rt) throw std::runtime_error( std::string( "user with id '") + username + "' is not defined in index");
	return rt;
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

void Storage::loadTermnoMap( const char* termnomap_source)
{
	leveldb::WriteBatch batch;
	KeyValueStorage termnostor( m_db, DatabaseKey::TermValuePrefix, false);

	m_termno_map = new VarSizeNodeTree();
	try
	{
		unsigned char const* si = (const unsigned char*)termnomap_source;
		std::string name;

		while (*si)
		{
			name.resize(0);
			for (; *si != '\n' && *si != '\r' && *si; ++si)
			{
				name.push_back( *si);
			}
			if (*si == '\r') ++si;
			if (*si == '\n') ++si;

			VarSizeNodeTree::NodeData dupkey;
			if (m_termno_map->find( name.c_str(), dupkey)) continue;

			Index termno = loadIndexValue( DatabaseKey::TermValuePrefix, name);
			if (!termno)
			{
				termno = allocTermno();
				std::string termnostr;
				packIndex( termnostr, termno);
				termnostor.store( name, termnostr, batch);
			}
			m_termno_map->set( name.c_str(), termno);
		}
		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->Write( options, &batch);
		batch.Clear();

		if (!status.ok())
		{
			throw std::runtime_error( std::string( "error writing leveldb database batch: ") + status.ToString());
		}
	}
	catch (const std::runtime_error& err)
	{
		throw std::runtime_error( std::string("failed to build termno map: ") + err.what());
	}
}

