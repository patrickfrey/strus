/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
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
#include "strus/documentTermIteratorInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/versionStorage.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "byteOrderMark.hpp"
#include "statisticsInitIterator.hpp"
#include "statisticsUpdateIterator.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "documentFrequencyCache.hpp"
#include "metaDataBlockCache.hpp"
#include "postingIterator.hpp"
#include "nullIterator.hpp"
#include "databaseAdapter.hpp"
#include "forwardIterator.hpp"
#include "documentTermIterator.hpp"
#include "indexPacker.hpp"
#include "attributeReader.hpp"
#include "keyAllocatorInterface.hpp"
#include "extractKeyValueData.hpp"
#include "valueIterator.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

using namespace strus;

void StorageClient::cleanup()
{
	if (m_metaDataBlockCache)
	{
		delete m_metaDataBlockCache; 
		m_metaDataBlockCache = 0;
	}
}

StorageClient::StorageClient(
		DatabaseClientInterface* database_,
		const char* termnomap_source,
		const StatisticsProcessorInterface* statisticsProc_,
		ErrorBufferInterface* errorhnd_)
	:m_database()
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_metaDataBlockCache(0)
	,m_statisticsProc(statisticsProc_)
	,m_errorhnd(errorhnd_)
{
	try
	{
		m_metadescr.load( database_);
		m_metaDataBlockCache = new MetaDataBlockCache( database_, m_metadescr);

		loadVariables( database_);
		if (termnomap_source) loadTermnoMap( termnomap_source);
		m_database.reset( database_);
	}
	catch (const std::bad_alloc& err)
	{
		cleanup();
		throw strus::runtime_error(_TXT("out of memory creating storage client"));
	}
	catch (const std::runtime_error& err)
	{
		cleanup();
		throw strus::runtime_error(_TXT("error creating storage client: %s"), err.what());
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
}

static Index versionNo( Index major, Index minor)
{
	return (major * 1000) + minor;
}

static unsigned int versionIndex( const Index& version)
{
	static const Index ar[] = {0004,0};
	unsigned int ii=0;
	for (;ar[ii] && ar[ii] < version; ++ii){}
	return ii;
}

void StorageClient::loadVariables( DatabaseClientInterface* database_)
{
	ByteOrderMark byteOrderMark;
	Index bom;
	ByteOrderMark storage_byteOrderMark;
	Index next_termno_;
	Index next_typeno_;
	Index next_docno_;
	Index next_attribno_;
	Index nof_documents_;
	Index next_userno_;
	Index version_;

	DatabaseAdapter_Variable::Reader varstor( database_);
	if (!varstor.load( "TermNo", next_termno_)
	||  !varstor.load( "TypeNo", next_typeno_)
	||  !varstor.load( "DocNo", next_docno_)
	||  !varstor.load( "AttribNo", next_attribno_)
	||  !varstor.load( "NofDocs", nof_documents_))
	{
		throw strus::runtime_error( _TXT( "corrupt storage, not all mandatory variables defined"));
	}
	if (!varstor.load( "Version", version_))
	{
		version_ = versionNo( 0, 4);
	}
	if (versionIndex( version_) != versionIndex( versionNo( STRUS_STORAGE_VERSION_MAJOR, STRUS_STORAGE_VERSION_MINOR)))
	{
		unsigned int major = version_ / 1000;
		unsigned int minor = version_ % 1000;
		throw strus::runtime_error( _TXT( "incompatible storage version %u.%u software is %u.%u. please rebuild your storage"), major, minor, (unsigned int)STRUS_STORAGE_VERSION_MAJOR, (unsigned int)STRUS_STORAGE_VERSION_MINOR);
	}
	(void)varstor.load( "UserNo", next_userno_);
	if (varstor.load( "ByteOrderMark", bom))
	{
		if (bom != byteOrderMark.value())
		{
			storage_byteOrderMark.set( bom);
			throw strus::runtime_error( _TXT( "incompatible platform for accessing this storage, storage created as %s, but accessed from a machine with %s"), storage_byteOrderMark.endianess(), byteOrderMark.endianess());
		}
	}
	m_next_termno.set( next_termno_);
	m_next_typeno.set( next_typeno_);
	m_next_docno.set( next_docno_);
	m_next_attribno.set( next_attribno_);
	m_nof_documents.set( nof_documents_);
	m_next_userno.set( next_userno_);
}

void StorageClient::storeVariables()
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	if (!transaction.get()) throw strus::runtime_error(_TXT("error storing variables"));
	getVariablesWriteBatch( transaction.get(), 0);
	transaction->commit();
}

void StorageClient::getVariablesWriteBatch(
		DatabaseTransactionInterface* transaction,
		int nof_documents_incr)
{
	DatabaseAdapter_Variable::Writer varstor( m_database.get());
	varstor.store( transaction, "TermNo", m_next_termno.value());
	varstor.store( transaction, "TypeNo", m_next_typeno.value());
	varstor.store( transaction, "DocNo", m_next_docno.value());
	varstor.store( transaction, "AttribNo", m_next_attribno.value());
	varstor.store( transaction, "NofDocs", m_nof_documents.value() + nof_documents_incr);
	if (withAcl())
	{
		varstor.store( transaction, "UserNo", m_next_userno.value());
	}
}

StorageClient::~StorageClient()
{
	try
	{
		storeVariables();
	}
	CATCH_ERROR_MAP( _TXT("error closing storage client: %s"), *m_errorhnd);
	cleanup();
}

Index StorageClient::getTermValue( const std::string& name) const
{
	return DatabaseAdapter_TermValue::Reader( m_database.get()).get( name);
}

Index StorageClient::getTermType( const std::string& name) const
{
	return DatabaseAdapter_TermType::Reader( m_database.get()).get( utils::tolower( name));
}

Index StorageClient::getDocno( const std::string& name) const
{
	return DatabaseAdapter_DocId::Reader( m_database.get()).get( name);
}

Index StorageClient::getUserno( const std::string& name) const
{
	return DatabaseAdapter_UserName::Reader( m_database.get()).get( name);
}

Index StorageClient::getAttributeno( const std::string& name) const
{
	return DatabaseAdapter_AttributeKey::Reader( m_database.get()).get( utils::tolower( name));
}

std::vector<std::string> StorageClient::getAttributeNames() const
{
	std::vector<std::string> rt;
	DatabaseAdapter_AttributeKey::Cursor attrcursor( m_database.get());
	Index attrno;
	std::string attrname;
	for (bool more=attrcursor.loadFirst( attrname, attrno); more;
		more=attrcursor.loadNext( attrname, attrno))
	{
		rt.push_back( attrname);
	}
	return rt;
}

PostingIteratorInterface*
	StorageClient::createTermPostingIterator(
		const std::string& typestr,
		const std::string& termstr) const
{
	try
	{
		Index typeno = getTermType( typestr);
		Index termno = getTermValue( termstr);
		if (!typeno || !termno)
		{
			return new NullIterator( typeno, termno, termstr.c_str());
		}
		return new PostingIterator( this, m_database.get(), typeno, termno, termstr.c_str(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term posting search index iterator: %s"), *m_errorhnd, 0);
}

ForwardIteratorInterface*
	StorageClient::createForwardIterator(
		const std::string& type) const
{
	try
	{
		return new ForwardIterator( this, m_database.get(), type, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating forward iterator: %s"), *m_errorhnd, 0);
}

DocumentTermIteratorInterface*
	StorageClient::createDocumentTermIterator(
		const std::string& type) const
{
	try
	{
		return new DocumentTermIterator( this, m_database.get(), type, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term occurrence iterator: %s"), *m_errorhnd, 0);
}

class InvertedAclIterator
	:public InvAclIteratorInterface
	,public IndexSetIterator
	
{
public:
	InvertedAclIterator( const DatabaseClientInterface* database_, const Index& userno_, ErrorBufferInterface* errorhnd_)
		:IndexSetIterator( database_, DatabaseKey::UserAclBlockPrefix, BlockKey(userno_), true),m_errorhnd(errorhnd_){}
	virtual ~InvertedAclIterator(){}

	virtual Index skipDoc( const Index& docno_)
	{
		try
		{
			return skip(docno_);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error in skip doc of inverted ACL iterator: %s"), *m_errorhnd, 0);
	}
private:
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
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
	try
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
			return new InvertedAclIterator( m_database.get(), userno, m_errorhnd);
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating inverted ACL iterator: %s"), *m_errorhnd, 0);
}


StorageTransactionInterface*
	StorageClient::createTransaction()
{
	try
	{
		if (m_statisticsProc && !m_statisticsBuilder.get())
		{
			StatisticsProcessorInterface::BuilderOptions options( StatisticsProcessorInterface::BuilderOptions::InsertInLexicalOrder);
			m_statisticsBuilder.reset( m_statisticsProc->createBuilder( options));
		}
		return new StorageTransaction( this, m_database.get(), &m_metadescr, m_next_typeno.value(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client transaction: %s"), *m_errorhnd, 0);
}

StorageDocumentInterface* 
	StorageClient::createDocumentChecker(
		const std::string& docid,
		const std::string& logfilename) const
{
	try
	{
		return new StorageDocumentChecker( this, m_database.get(), docid, logfilename, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document checker: %s"), *m_errorhnd, 0);
}

StatisticsBuilderInterface* StorageClient::getStatisticsBuilder()
{
	return m_statisticsBuilder.get();
}

void StorageClient::declareNofDocumentsInserted( int incr)
{
	m_nof_documents.increment( incr);
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
		throw strus::logic_error( _TXT( "cannot use %s allocator for non immediate alloc"), "typeno");
	}

private:
	StorageClient* m_storage;
};

class DocnoAllocator
	:public KeyAllocatorInterface
{
public:
	DocnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(false),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name, bool& isNew)
	{
		throw strus::logic_error( _TXT( "cannot use %s allocator for immediate alloc"), "docno");
	}
	virtual Index alloc()
	{
		return m_storage->allocDocno();
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
			throw strus::runtime_error( _TXT( "storage configured without ACL. No users can be created"));
		}
		return m_storage->allocUsernoImm( name, isNew);
	}
	virtual Index alloc()
	{
		throw strus::logic_error( _TXT( "cannot use %s allocator for non immediate alloc"), "userno");
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
		throw strus::logic_error( _TXT( "cannot use %s allocator for non immediate alloc"), "attribno");
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
		throw strus::logic_error( _TXT( "cannot use %s allocator for immediate alloc"), "termno");
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
	return m_next_userno.value() != 0;
}

Index StorageClient::allocTermno()
{
	return m_next_termno.allocIncrement();
}

Index StorageClient::allocDocno()
{
	return m_next_docno.allocIncrement();
}

Index StorageClient::allocTypenoImm( const std::string& name, bool& isNew)
{
	Index rt;
	DatabaseAdapter_TermType::ReadWriter stor(m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_typeno.allocIncrement());
		isNew = true;
	}
	return rt;
}

Index StorageClient::allocUsernoImm( const std::string& name, bool& isNew)
{
	Index rt;
	DatabaseAdapter_UserName::ReadWriter stor( m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_userno.allocIncrement());
		isNew = true;
	}
	return rt;
}

Index StorageClient::allocAttribnoImm( const std::string& name, bool& isNew)
{
	Index rt;
	DatabaseAdapter_AttributeKey::ReadWriter stor( m_database.get());
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_attribno.allocIncrement());
		isNew = true;
	}
	return rt;
}

Index StorageClient::maxTermTypeNo() const
{
	return m_next_typeno.value() -1;
}

IndexSetIterator StorageClient::getAclIterator( const Index& docno) const
{
	return IndexSetIterator( m_database.get(), DatabaseKey::AclBlockPrefix, BlockKey(docno), false);
}

IndexSetIterator StorageClient::getUserAclIterator( const Index& userno) const
{
	return IndexSetIterator( m_database.get(), DatabaseKey::UserAclBlockPrefix, BlockKey(userno), true);
}

Index StorageClient::nofAttributeTypes()
{
	return m_next_attribno.value();
}

Index StorageClient::nofDocumentsInserted() const
{
	return m_nof_documents.value();
}

Index StorageClient::documentFrequency( const Index& typeno, const Index& termno) const
{
	return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
}

Index StorageClient::documentFrequency( const std::string& type, const std::string& term) const
{
	try
	{
		Index typeno = getTermValue( type);
		Index termno = getTermValue( term);
		return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error evaluating term document frequency: %s"), *m_errorhnd, 0);
}

Index StorageClient::maxDocumentNumber() const
{
	return m_next_docno.value()-1;
}

Index StorageClient::documentNumber( const std::string& docid) const
{
	return getDocno( docid);
}

ValueIteratorInterface* StorageClient::createTermTypeIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_TermType>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term type iterator: %s"), *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createTermValueIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_TermValue>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term value iterator: %s"), *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createDocIdIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_DocId>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document id iterator: %s"), *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createUserNameIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_UserName>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating user name iterator: %s"), *m_errorhnd, 0);
}

Index StorageClient::documentStatistics(
		const Index& docno,
		const DocumentStatisticsType& stat,
		const std::string& type) const
{
	try
	{
		Index rt = 0;
		Index typeno = getTermType( type);
		if (typeno == 0) return 0;
	
		DatabaseAdapter_InverseTerm::Reader dbadapter_inv( m_database.get());
		InvTermBlock invblk;
		typedef InvTermBlock::Element InvTerm;
	
		if (dbadapter_inv.load( docno, invblk))
		{
			char const* ei = invblk.begin();
			const char* ee = invblk.end();
			switch (stat)
			{
				case StatNofTerms:
					for (;ei != ee; ei = invblk.next( ei))
					{
						InvTerm it = invblk.element_at( ei);
						if (typeno == it.typeno)
						{
							rt += 1;
						}
					}
				break;
				case StatNofTermOccurrencies:
					for (;ei != ee; ei = invblk.next( ei))
					{
						InvTerm it = invblk.element_at( ei);
						if (typeno == it.typeno)
						{
							rt += it.ff;
						}
					}
				break;
			}
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error evaluating document statistics: %s"), *m_errorhnd, 0);
}

Index StorageClient::userId( const std::string& username) const
{
	Index rt = getUserno( username);
	if (!rt) throw strus::runtime_error( _TXT( "user with id '%s' is not defined in index"), username.c_str());
	return rt;
}

AttributeReaderInterface* StorageClient::createAttributeReader() const
{
	try
	{
		return new AttributeReader( this, m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating attribute reader: %s"), *m_errorhnd, 0);
}

MetaDataReaderInterface* StorageClient::createMetaDataReader() const
{
	try
	{
		return new MetaDataReader( m_metaDataBlockCache, &m_metadescr, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating meta data reader: %s"), *m_errorhnd, 0);
}

void StorageClient::loadTermnoMap( const char* termnomap_source)
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	if (!transaction.get()) throw strus::runtime_error(_TXT("error loading termno map"));
	utils::UnorderedMap<std::string,Index> termno_map;
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
			if (termno_map.find( name) != termno_map.end()) continue;

			// [3] Check, if already defined in storage:
			DatabaseAdapter_TermValue::ReadWriter stor(m_database.get());
			Index termno = stor.get( name);
			if (!termno)
			{
				// ... create it if not
				termno = allocTermno();
				stor.store( transaction.get(), name, termno);
			}
			// [4] Register it in the map:
			termno_map[ name] = termno;
		}
		transaction->commit();
	}
	catch (const std::runtime_error& err)
	{
		throw strus::runtime_error( _TXT( "failed to build termno map: "), err.what());
	}
}

void StorageClient::fillDocumentFrequencyCache()
{
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

DocumentFrequencyCache* StorageClient::getDocumentFrequencyCache()
{
	return m_documentFrequencyCache.get();
}

bool StorageClient::fetchPeerUpdateMessage( const char*& msg, std::size_t& msgsize)
{
	TransactionLock lock( this);
	return m_statisticsBuilder->fetchMessage( msg, msgsize);
}

StatisticsIteratorInterface* StorageClient::createInitStatisticsIterator( bool sign)
{
	try
	{
		if (!m_statisticsProc)
		{
			throw strus::runtime_error(_TXT( "no peer message processor defined"));
		}
		{
			TransactionLock lock( this);
			if (!m_statisticsBuilder.get())
			{
				StatisticsProcessorInterface::BuilderOptions options( StatisticsProcessorInterface::BuilderOptions::InsertInLexicalOrder);
				m_statisticsBuilder.reset( m_statisticsProc->createBuilder( options));
			}
		}
		return new StatisticsInitIterator( this, m_database.get(), sign, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating peer message iterator: %s"), *m_errorhnd, 0);
}

StatisticsIteratorInterface* StorageClient::createUpdateStatisticsIterator()
{
	try
	{
		if (!m_statisticsProc)
		{
			throw strus::runtime_error(_TXT( "no peer message processor defined"));
		}
		{
			TransactionLock lock( this);
			if (!m_statisticsBuilder.get())
			{
				StatisticsProcessorInterface::BuilderOptions options( StatisticsProcessorInterface::BuilderOptions::InsertInLexicalOrder);
				m_statisticsBuilder.reset( m_statisticsProc->createBuilder( options));
			}
		}
		return new StatisticsUpdateIterator( this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating peer message iterator: %s"), *m_errorhnd, 0);
}

const StatisticsProcessorInterface*  StorageClient::getStatisticsProcessor() const
{
	return m_statisticsProc;
}

static std::string keystring( const strus::DatabaseCursorInterface::Slice& key)
{
	const char hex[] = "0123456789abcdef";
	std::string rt;
	char const* ki = key.ptr();
	char const* ke = key.ptr()+key.size();
	for (; ki != ke; ++ki)
	{
		if ((unsigned char)*ki > 32 && (unsigned char)*ki < 128)
		{
			rt.push_back( *ki);
		}
		else
		{
			rt.push_back( '[');
			rt.push_back( hex[ (unsigned char)*ki / 16]);
			rt.push_back( hex[ (unsigned char)*ki % 16]);
			rt.push_back( ']');
		}
	}
	return rt;
}

static void checkKeyValue(
		const strus::DatabaseClientInterface* database,
		const strus::DatabaseCursorInterface::Slice& key,
		const strus::DatabaseCursorInterface::Slice& value,
		std::ostream& errorlog)
{
	try
	{
		switch (key.ptr()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				strus::TermTypeData( key, value);
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				strus::TermValueData( key, value);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData( key, value);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData( key, value);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData( key, value);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::MetaDataDescription metadescr( database);
				strus::DocMetaDataData( &metadescr, key, value);
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				strus::DocAttributeData( key, value);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData( key, value);
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData( key, value);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData( key, value);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData( key, value);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData( key, value);
				break;
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		std::string ks( keystring( key));
		char buf[ 256];
		snprintf( buf, 256, _TXT( "error in checked key '%s':"), ks.c_str());
		errorlog << buf << err.what() << std::endl;
	}
}

bool StorageClient::checkStorage( std::ostream& errorlog) const
{
	try
	{
		std::auto_ptr<strus::DatabaseCursorInterface>
			cursor( m_database->createCursor( strus::DatabaseOptions()));
		if (!cursor.get()) return false;

		strus::DatabaseCursorInterface::Slice key = cursor->seekFirst( 0, 0);
	
		for (; key.defined(); key = cursor->seekNext())
		{
			if (key.size() == 0)
			{
				m_errorhnd->report( _TXT( "found empty key in storage"));
				return false;
			}
			checkKeyValue( m_database.get(), key, cursor->value(), errorlog);
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error checking storage: %s"), *m_errorhnd, false);
}

static void dumpKeyValue(
		std::ostream& out,
		const strus::DatabaseClientInterface* database,
		const strus::DatabaseCursorInterface::Slice& key,
		const strus::DatabaseCursorInterface::Slice& value)
{
	try
	{
		switch (key.ptr()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				strus::TermTypeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				strus::TermValueData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::MetaDataDescription metadescr( database);
				strus::DocMetaDataData data( &metadescr, key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				strus::DocAttributeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData data( key, value);
				data.print( out);
				break;
			}
			default:
			{
				throw strus::runtime_error( _TXT( "illegal data base prefix"));
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		std::string ks( keystring( key));
		throw strus::runtime_error( _TXT( "error in dumped dkey '%s': %s"), ks.c_str(), err.what());
	}
}

class StorageDump
	:public StorageDumpInterface
{
public:
	StorageDump( const strus::DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_)
		:m_database(database_)
		,m_cursor( database_->createCursor( strus::DatabaseOptions()))
		,m_errorhnd(errorhnd_)
	{
		if (!m_cursor.get()) throw strus::runtime_error(_TXT("error creating database cursor"));
		m_key = m_cursor->seekFirst( 0, 0);
	}
	virtual ~StorageDump(){}

	virtual bool nextChunk( const char*& chunk, std::size_t& chunksize)
	{
		try
		{
			unsigned int ii = 0, nn = NofKeyValuePairsPerChunk;
			std::ostringstream output;
			for (; m_key.defined() && ii<nn; m_key = m_cursor->seekNext(),++ii)
			{
				if (m_key.size() == 0)
				{
					throw strus::runtime_error( _TXT( "found empty key in storage"));
				}
				dumpKeyValue( output, m_database, m_key, m_cursor->value());
			};
			m_chunk = output.str();
			chunk = m_chunk.c_str();
			chunksize = m_chunk.size();
			return (chunksize != 0);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error fetching next chunk of storage dump: %s"), *m_errorhnd, false);
	}

	/// \brief How many key/value pairs to return in one chunk
	enum {NofKeyValuePairsPerChunk=256};

private:
	const strus::DatabaseClientInterface* m_database;
	std::auto_ptr<strus::DatabaseCursorInterface> m_cursor;
	strus::DatabaseCursorInterface::Slice m_key;
	std::string m_chunk;
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

StorageDumpInterface* StorageClient::createDump() const
{
	try
	{
		return new StorageDump( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage dump interface: %s"), *m_errorhnd, 0);
}



