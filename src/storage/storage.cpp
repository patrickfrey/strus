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
#include "storage.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseKey.hpp"
#include "databaseRecord.hpp"
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

using namespace strus;

void Storage::cleanup()
{
	if (m_metaDataBlockCache)
	{
		delete m_metaDataBlockCache; 
		m_metaDataBlockCache = 0;
	}
	if (m_termno_map)
	{
		delete m_termno_map;
		m_termno_map = 0;
	}
}

Storage::Storage( DatabaseInterface* database_, const char* termnomap_source)
	:m_database(database_)
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_global_nof_documents(0)
	,m_transactionCnt(0)
	,m_metaDataBlockCache(0)
	,m_termno_map(0)
{
	try
	{
		m_metadescr.load( database_);
		m_metaDataBlockCache = new MetaDataBlockCache( m_database, m_metadescr);

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
	if (!DatabaseRecord_Variable::load( m_database, "TermNo", m_next_termno)
	||  !DatabaseRecord_Variable::load( m_database, "TypeNo", m_next_typeno)
	||  !DatabaseRecord_Variable::load( m_database, "DocNo", m_next_docno)
	||  !DatabaseRecord_Variable::load( m_database, "AttribNo", m_next_attribno)
	||  !DatabaseRecord_Variable::load( m_database, "NofDocs", m_nof_documents))
	{
		throw std::runtime_error( "corrupt storage, not all mandatory variables defined");
	}
	(void)DatabaseRecord_Variable::load( m_database, "UserNo", m_next_userno);
}

void Storage::storeVariables()
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());

	DatabaseRecord_Variable::store( transaction.get(), "TermNo", m_next_termno);
	DatabaseRecord_Variable::store( transaction.get(), "TypeNo", m_next_typeno);
	DatabaseRecord_Variable::store( transaction.get(), "DocNo", m_next_docno);
	DatabaseRecord_Variable::store( transaction.get(), "AttribNo", m_next_attribno);
	DatabaseRecord_Variable::store( transaction.get(), "NofDocs", m_nof_documents);
	if (withAcl())
	{
		DatabaseRecord_Variable::store( transaction.get(), "UserNo", m_next_userno);
	}
	transaction->commit();
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

Index Storage::getTermValue( const std::string& name) const
{
	if (m_termno_map)
	{
		VarSizeNodeTree::NodeData cached_termno;
		if (m_termno_map->find( name.c_str(), cached_termno)) return cached_termno;
	}
	return DatabaseRecord_TermValue::get( m_database, name);
}

Index Storage::getTermType( const std::string& name) const
{
	return DatabaseRecord_TermType::get( m_database, name);
}

Index Storage::getDocno( const std::string& name) const
{
	return DatabaseRecord_DocId::get( m_database, name);
}

Index Storage::getUserno( const std::string& name) const
{
	return DatabaseRecord_UserName::get( m_database, name);
}

Index Storage::getAttributeName( const std::string& name) const
{
	return DatabaseRecord_AttributeKey::get( m_database, name);
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
	return new PostingIterator( m_database, typeno, termno, termstr.c_str());
}

ForwardIteratorInterface*
	Storage::createForwardIterator(
		const std::string& type) const
{
	return new ForwardIterator( this, m_database, type);
}

class InvertedAclIterator
	:public InvAclIteratorInterface
	,public IndexSetIterator
	
{
public:
	InvertedAclIterator( DatabaseInterface* database_, const Index& userno_)
		:IndexSetIterator( database_, DatabaseKey::UserAclBlockPrefix, BlockKey(userno_)){}
	virtual ~InvertedAclIterator(){}

	virtual Index skipDoc( const Index& docno_)
	{
		return skip(docno_);
	}
};

class UnknownUserInvertedAclIterator
	:public InvAclIteratorInterface
{
public:
	UnknownUserInvertedAclIterator(){}
	virtual Index skipDoc( const Index&)	{return 0;}
};

InvAclIteratorInterface*
	Storage::createInvAclIterator(
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
		return new InvertedAclIterator( m_database, userno);
	}
}


StorageTransactionInterface*
	Storage::createTransaction()
{
	{
		boost::mutex::scoped_lock lock( m_transactionCnt_mutex);
		++m_transactionCnt;
	}
	return new StorageTransaction( this, m_database, &m_metadescr, m_termno_map);
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
	m_global_nof_documents += value;
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
	Index rt;
	if (!DatabaseRecord_TermType::load( m_database, name, rt))
	{
		DatabaseRecord_TermType::storeImm( m_database, name, rt = m_next_typeno++);
		isNew = true;
	}
	return rt;
}

Index Storage::allocDocnoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_docno);
	Index rt;
	if (!DatabaseRecord_DocId::load( m_database, name, rt))
	{
		DatabaseRecord_DocId::storeImm( m_database, name, rt = m_next_docno++);
		isNew = true;
	}
	return rt;
}

Index Storage::allocUsernoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_userno);
	Index rt;
	if (!DatabaseRecord_UserName::load( m_database, name, rt))
	{
		DatabaseRecord_UserName::storeImm( m_database, name, rt = m_next_userno++);
		isNew = true;
	}
	return rt;
}

Index Storage::allocAttribnoIm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_attribno);
	Index rt;
	if (!DatabaseRecord_AttributeKey::load( m_database, name, rt))
	{
		DatabaseRecord_AttributeKey::storeImm( m_database, name, rt = m_next_attribno++);
		isNew = true;
	}
	return rt;
}

IndexSetIterator Storage::getAclIterator( const Index& docno) const
{
	return IndexSetIterator( m_database, DatabaseKey::AclBlockPrefix, BlockKey(docno));
}

IndexSetIterator Storage::getUserAclIterator( const Index& userno) const
{
	return IndexSetIterator( m_database, DatabaseKey::UserAclBlockPrefix, BlockKey(userno));
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
	return new AttributeReader( this, m_database);
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
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	m_termno_map = new VarSizeNodeTree();
	try
	{
		unsigned char const* si = (const unsigned char*)termnomap_source;
		std::string name;

		while (*si)
		{
			// [1] Fetch next term string to cache (one line in 'termnomap_source'):
			name.resize(0);
			for (; *si != '\n' && *si != '\r' && *si; ++si)
			{
				name.push_back( *si);
			}
			if (*si == '\r') ++si;
			if (*si == '\n') ++si;

			// [2] Check, if already loaded:
			VarSizeNodeTree::NodeData dupkey;
			if (m_termno_map->find( name.c_str(), dupkey)) continue;

			// [3] Check, if already defined in storage:
			Index termno = DatabaseRecord_TermValue::get( m_database, name);
			if (!termno)
			{
				// ... create it if not
				termno = allocTermno();
				DatabaseRecord_TermValue::store( transaction.get(), name, termno);
			}
			// [4] Register it in the map:
			m_termno_map->set( name.c_str(), termno);
		}
		transaction->commit();
	}
	catch (const std::runtime_error& err)
	{
		throw std::runtime_error( std::string("failed to build termno map: ") + err.what());
	}
}

