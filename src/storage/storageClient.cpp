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
#include "storageClient.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/peerStorageTransactionInterface.hpp"
#include "strus/storagePeerInterface.hpp"
#include "strus/storagePeerTransactionInterface.hpp"
#include "strus/reference.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "peerStorageTransaction.hpp"
#include "documentFrequencyCache.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseAdapter.hpp"
#include "forwardIterator.hpp"
#include "indexPacker.hpp"
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

void StorageClient::cleanup()
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

StorageClient::StorageClient( DatabaseClientInterface* database_, const char* termnomap_source)
	:m_database(database_)
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_global_nof_documents(0)
	,m_metaDataBlockCache(0)
	,m_termno_map(0)
	,m_storagePeer(0)
{
	try
	{
		m_metadescr.load( database_);
		m_metaDataBlockCache = new MetaDataBlockCache( m_database.get(), m_metadescr);

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

void StorageClient::releaseTransaction( const std::vector<Index>& refreshList)
{
	// Refresh all entries touched by the inserts/updates written
	std::vector<Index>::const_iterator ri = refreshList.begin(), re = refreshList.end();
	for (; ri != re; ++ri)
	{
		m_metaDataBlockCache->declareVoid( *ri);
	}
	m_metaDataBlockCache->refresh();

	storeVariables();
}

void StorageClient::loadVariables()
{
	DatabaseAdapter_Variable varstor( m_database.get());
	if (!varstor.load( "TermNo", m_next_termno)
	||  !varstor.load( "TypeNo", m_next_typeno)
	||  !varstor.load( "DocNo", m_next_docno)
	||  !varstor.load( "AttribNo", m_next_attribno)
	||  !varstor.load( "NofDocs", m_nof_documents)
	)
	{
		throw std::runtime_error( "corrupt storage, not all mandatory variables defined");
	}
	(void)varstor.load( "UserNo", m_next_userno);
	m_global_nof_documents = m_nof_documents;
}

void StorageClient::storeVariables()
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	getVariablesWriteBatch( transaction.get(), 0);
	transaction->commit();
}

void StorageClient::getVariablesWriteBatch(
		DatabaseTransactionInterface* transaction,
		int nof_documents_incr)
{
	DatabaseAdapter_Variable varstor( m_database.get());
	varstor.store( transaction, "TermNo", m_next_termno);
	varstor.store( transaction, "TypeNo", m_next_typeno);
	varstor.store( transaction, "DocNo", m_next_docno);
	varstor.store( transaction, "AttribNo", m_next_attribno);
	varstor.store( transaction, "NofDocs", m_nof_documents + nof_documents_incr);
	if (withAcl())
	{
		varstor.store( transaction, "UserNo", m_next_userno);
	}
}

void StorageClient::close()
{
	storeVariables();
}

StorageClient::~StorageClient()
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

Index StorageClient::getTermValue( const std::string& name) const
{
	if (m_termno_map)
	{
		VarSizeNodeTree::NodeData cached_termno;
		if (m_termno_map->find( name.c_str(), cached_termno)) return cached_termno;
	}
	return DatabaseAdapter_TermValue( m_database.get()).get( name);
}

Index StorageClient::getTermType( const std::string& name) const
{
	return DatabaseAdapter_TermType( m_database.get()).get( boost::algorithm::to_lower_copy( name));
}

Index StorageClient::getDocno( const std::string& name) const
{
	return DatabaseAdapter_DocId( m_database.get()).get( name);
}

Index StorageClient::getUserno( const std::string& name) const
{
	return DatabaseAdapter_UserName( m_database.get()).get( name);
}

Index StorageClient::getAttributeName( const std::string& name) const
{
	return DatabaseAdapter_AttributeKey( m_database.get()).get( boost::algorithm::to_lower_copy( name));
}

GlobalCounter StorageClient::documentFrequency( const Index& typeno, const Index& termno) const
{
	if (m_documentFrequencyCache.get())
	{
		return m_documentFrequencyCache->getValue( typeno, termno);
	}
	else
	{
		return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
	}
}

PostingIteratorInterface*
	StorageClient::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr) const
{
	Index typeno = getTermType( typestr);
	Index termno = getTermValue( termstr);
	if (!typeno || !termno)
	{
		return new NullIterator( typeno, termno, termstr.c_str());
	}
	return new PostingIterator( this, m_database.get(), typeno, termno, termstr.c_str());
}

ForwardIteratorInterface*
	StorageClient::createForwardIterator(
		const std::string& type) const
{
	return new ForwardIterator( this, m_database.get(), type);
}

class InvertedAclIterator
	:public InvAclIteratorInterface
	,public IndexSetIterator
	
{
public:
	InvertedAclIterator( DatabaseClientInterface* database_, const Index& userno_)
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
	StorageClient::createInvAclIterator(
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
		return new InvertedAclIterator( m_database.get(), userno);
	}
}


StorageTransactionInterface*
	StorageClient::createTransaction()
{
	return new StorageTransaction( this, m_database.get(), m_storagePeer, &m_metadescr, m_termno_map);
}

StorageDocumentInterface* 
	StorageClient::createDocumentChecker(
		const std::string& docid,
		const std::string& logfilename) const
{
	return new StorageDocumentChecker( this, m_database.get(), docid, logfilename);
}

Index StorageClient::allocDocnoRange( std::size_t nofDocuments)
{
	boost::mutex::scoped_lock lock( m_mutex_docno);
	Index rt = m_next_docno;
	m_next_docno += nofDocuments;
	if (m_next_docno <= rt) throw std::runtime_error( "docno allocation error");
	return rt;
}

void StorageClient::declareNofDocumentsInserted( int incr)
{
	boost::mutex::scoped_lock lock( m_nof_documents_mutex);
	m_nof_documents += incr;
	m_global_nof_documents += incr;
}

class TypenoAllocator
	:public KeyAllocatorInterface
{
public:
	TypenoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocTypenoImm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use typeno allocator for non immediate alloc");
	}

private:
	StorageClient* m_storage;
};

class DocnoAllocator
	:public KeyAllocatorInterface
{
public:
	DocnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocDocnoImm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use docno allocator for non immediate alloc");
	}
private:
	StorageClient* m_storage;
};

class UsernoAllocator
	:public KeyAllocatorInterface
{
public:
	UsernoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		if (!m_storage->withAcl())
		{
			throw std::runtime_error( "storage configured without ACL. No users can be created");
		}
		return m_storage->allocUsernoImm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use docno allocator for non immediate alloc");
	}
private:
	StorageClient* m_storage;
};

class AttribnoAllocator
	:public KeyAllocatorInterface
{
public:
	AttribnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		return m_storage->allocAttribnoImm( name, isNew);
	}
	virtual Index alloc()
	{
		throw std::logic_error("cannot use attribno allocator for non immediate alloc");
	}
private:
	StorageClient* m_storage;
};

class TermnoAllocator
	:public KeyAllocatorInterface
{
public:
	TermnoAllocator( StorageClient* storage_)
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
	StorageClient* m_storage;
};


KeyAllocatorInterface* StorageClient::createTypenoAllocator()
{
	return new TypenoAllocator( this);
}

KeyAllocatorInterface* StorageClient::createDocnoAllocator()
{
	return new DocnoAllocator( this);
}

KeyAllocatorInterface* StorageClient::createUsernoAllocator()
{
	return new UsernoAllocator( this);
}

KeyAllocatorInterface* StorageClient::createAttribnoAllocator()
{
	return new AttribnoAllocator( this);
}

KeyAllocatorInterface* StorageClient::createTermnoAllocator()
{
	return new TermnoAllocator( this);
}

bool StorageClient::withAcl() const
{
	return m_next_userno != 0;
}

Index StorageClient::allocTermno()
{
	boost::mutex::scoped_lock lock( m_mutex_termno);
	return m_next_termno++;
}

Index StorageClient::allocTypenoImm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_typeno);
	Index rt;
	DatabaseAdapter_TermType stor(m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_typeno++);
		isNew = true;
	}
	return rt;
}

Index StorageClient::allocDocnoImm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_docno);
	Index rt;
	DatabaseAdapter_DocId stor(m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_docno++);
		isNew = true;
	}
	return rt;
}

Index StorageClient::allocUsernoImm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_userno);
	Index rt;
	DatabaseAdapter_UserName stor( m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_userno++);
		isNew = true;
	}
	return rt;
}

Index StorageClient::allocAttribnoImm( const std::string& name, bool& isNew)
{
	boost::mutex::scoped_lock lock( m_mutex_attribno);
	Index rt;
	DatabaseAdapter_AttributeKey stor( m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_attribno++);
		isNew = true;
	}
	return rt;
}

IndexSetIterator StorageClient::getAclIterator( const Index& docno) const
{
	return IndexSetIterator( m_database.get(), DatabaseKey::AclBlockPrefix, BlockKey(docno));
}

IndexSetIterator StorageClient::getUserAclIterator( const Index& userno) const
{
	return IndexSetIterator( m_database.get(), DatabaseKey::UserAclBlockPrefix, BlockKey(userno));
}

Index StorageClient::nofAttributeTypes()
{
	return m_next_termno -1;
}

GlobalCounter StorageClient::globalNofDocumentsInserted() const
{
	return m_global_nof_documents;
}

Index StorageClient::localNofDocumentsInserted() const
{
	return m_nof_documents;
}

GlobalCounter StorageClient::globalDocumentFrequency(
		const std::string& type,
		const std::string& term) const
{
	Index typeno = getTermValue( type);
	Index termno = getTermValue( term);

	if (m_documentFrequencyCache.get())
	{
		return m_documentFrequencyCache->getValue( typeno, termno);
	}
	else
	{
		return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
	}
}

Index StorageClient::localDocumentFrequency(
		const std::string& type,
		const std::string& term) const
{
	Index typeno = getTermValue( type);
	Index termno = getTermValue( term);
	return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
}

Index StorageClient::maxDocumentNumber() const
{
	return m_next_docno-1;
}

Index StorageClient::documentNumber( const std::string& docid) const
{
	Index rt = getDocno( docid);
	if (!rt) throw std::runtime_error( std::string( "document with id '") + docid + "' is not defined in index");
	return rt;
}

Index StorageClient::userId( const std::string& username) const
{
	Index rt = getUserno( username);
	if (!rt) throw std::runtime_error( std::string( "user with id '") + username + "' is not defined in index");
	return rt;
}

AttributeReaderInterface* StorageClient::createAttributeReader() const
{
	return new AttributeReader( this, m_database.get());
}

MetaDataReaderInterface* StorageClient::createMetaDataReader() const
{
	return new MetaDataReader( m_metaDataBlockCache, &m_metadescr);
}

void StorageClient::loadTermnoMap( const char* termnomap_source)
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
			DatabaseAdapter_TermValue stor(m_database.get());
			Index termno = stor.get( name);
			if (!termno)
			{
				// ... create it if not
				termno = allocTermno();
				stor.store( transaction.get(), name, termno);
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

void StorageClient::declareGlobalNofDocumentsInserted( const GlobalCounter& incr)
{
	boost::mutex::scoped_lock lock( m_nof_documents_mutex);
	m_global_nof_documents += incr;
}

void StorageClient::fillDocumentFrequencyCache()
{
	TransactionLock lock( this);
	
	DocumentFrequencyCache::Batch dfbatch;
	DatabaseAdapter_DocFrequency::Cursor dfcursor( m_database.get());
	Index typeno;
	Index termno;
	Index df;
	for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
		more=dfcursor.loadNext( typeno, termno, df))
	{
		dfbatch.put( typeno, termno, df);
	}
	m_documentFrequencyCache.reset( new DocumentFrequencyCache());
	m_documentFrequencyCache->writeBatch( dfbatch);
}

PeerStorageTransactionInterface* StorageClient::createPeerStorageTransaction()
{
	if (!m_documentFrequencyCache.get())
	{
		fillDocumentFrequencyCache();
	}
	return new PeerStorageTransaction( this, m_database.get(), m_documentFrequencyCache.get());
}

void StorageClient::defineStoragePeerInterface(
		const StoragePeerInterface* storagePeer,
		bool doPopulateInitialState)
{
	if (!m_documentFrequencyCache.get())
	{
		fillDocumentFrequencyCache();
	}

	TransactionLock lock( this);
	m_storagePeer = storagePeer;

	// Fill a map with the strings of all types and terms in the collection:
	std::map<Index,std::string> typenomap;
	std::map<Index,std::string> termnomap;
	{
		DatabaseAdapter_TermType::Cursor typecursor( m_database.get());
		Index typeno;
		std::string typestr;
		for (bool more=typecursor.loadFirst( typestr, typeno); more;
			more=typecursor.loadNext( typestr, typeno))
		{
			typenomap[ typeno] = typestr;
		}
	}
	{
		DatabaseAdapter_TermValue::Cursor termcursor( m_database.get());
		Index termno;
		std::string termstr;
		for (bool more=termcursor.loadFirst( termstr, termno); more;
			more=termcursor.loadNext( termstr, termno))
		{
			termnomap[ termno] = termstr;
		}
	}
	// Populate the df's and the number of documents stored through the interface:
	{
		Reference<StoragePeerTransactionInterface> transaction( storagePeer->createTransaction());
		DatabaseAdapter_DocFrequency::Cursor dfcursor( m_database.get());
		Index typeno;
		Index termno;
		Index df;
		for (bool more=dfcursor.loadFirst( typeno, termno, df); more;
			more=dfcursor.loadNext( typeno, termno, df))
		{
			std::map<Index,std::string>::const_iterator ti = typenomap.find( typeno);
			if (ti == typenomap.end()) throw std::runtime_error( "undefined type in df key");
			std::map<Index,std::string>::const_iterator vi = termnomap.find( termno);
			if (ti == typenomap.end()) throw std::runtime_error( "undefined term in df key");

			if (!doPopulateInitialState) df = 0;

			transaction->populateDocumentFrequencyChange(
				ti->second.c_str(), vi->second.c_str(),
				df, true/*isNew*/);
		}
		transaction->populateNofDocumentsInsertedChange( m_nof_documents);
		transaction->try_commit();
		transaction->final_commit();
	}
}


