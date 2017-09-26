/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageClient.hpp"
#include "strus/databaseInterface.hpp"
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
#include "strus/storageInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/reference.hpp"
#include "strus/base/local_ptr.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "byteOrderMark.hpp"
#include "statisticsInitIterator.hpp"
#include "statisticsUpdateIterator.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "extractKeyValueData.hpp"
#include "documentFrequencyCache.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataRestriction.hpp"
#include "metaDataReader.hpp"
#include "postingIterator.hpp"
#include "browsePostingIterator.hpp"
#include "metaDataRangePostingIterator.hpp"
#include "nullPostingIterator.hpp"
#include "databaseAdapter.hpp"
#include "forwardIterator.hpp"
#include "documentTermIterator.hpp"
#include "indexPacker.hpp"
#include "attributeReader.hpp"
#include "keyAllocatorInterface.hpp"
#include "valueIterator.hpp"
#include "aclReader.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

using namespace strus;

#define MODULENAME "storageClient"

void StorageClient::cleanup()
{
	if (m_metaDataBlockCache)
	{
		delete m_metaDataBlockCache; 
		m_metaDataBlockCache = 0;
	}
}

StorageClient::StorageClient(
		const DatabaseInterface* database_,
		const std::string& databaseConfig,
		const char* termnomap_source,
		const StatisticsProcessorInterface* statisticsProc_,
		ErrorBufferInterface* errorhnd_)
	:m_database(database_->createClient( databaseConfig))
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_metaDataBlockCache(0)
	,m_statisticsProc(statisticsProc_)
	,m_close_called(false)
	,m_errorhnd(errorhnd_)
{
	try
	{
		if (!m_database.get()) throw strus::runtime_error(_TXT("failed to create database client: %s"), m_errorhnd->fetchError());
		m_metadescr.load( m_database.get());
		m_metaDataBlockCache = new MetaDataBlockCache( m_database.get(), m_metadescr);

		loadVariables( m_database.get());
		if (termnomap_source) loadTermnoMap( termnomap_source);
	}
	catch (const std::bad_alloc& err)
	{
		cleanup();
		throw strus::runtime_error( "%s", _TXT("out of memory creating storage client"));
	}
	catch (const std::runtime_error& err)
	{
		cleanup();
		throw strus::runtime_error(_TXT("error creating storage client: %s"), err.what());
	}
}

std::string StorageClient::config() const
{
	try
	{
		std::string rt( m_database->config());
		std::string mdstr( m_metadescr.tostring());
		if (!mdstr.empty())
		{
			if (!rt.empty()) rt.push_back(';');
			rt.append( "metadata=");
			rt.append( mdstr);
		}
		if (withAcl())
		{
			if (!rt.empty()) rt.push_back(';');
			rt.append( "acl=true");
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' mapping configuration to string: %s"), MODULENAME, *m_errorhnd, std::string());
}

void StorageClient::releaseTransaction( const std::vector<Index>& refreshList)
{
	if (m_metaDataBlockCache)
	{
		// Refresh all entries touched by the inserts/updates written
		std::vector<Index>::const_iterator ri = refreshList.begin(), re = refreshList.end();
		for (; ri != re; ++ri)
		{
			m_metaDataBlockCache->declareVoid( *ri);
		}
		m_metaDataBlockCache->refresh();
	}
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
		throw strus::runtime_error( "%s",  _TXT( "corrupt storage, not all mandatory variables defined"));
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
	if (!transaction.get()) throw strus::runtime_error( "%s", _TXT("error storing variables"));
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
	if (!m_close_called) try
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
		const std::string& termstr,
		const Index& length) const
{
	try
	{
		Index typeno = getTermType( typestr);
		Index termno = getTermValue( termstr);
		if (!typeno || !termno)
		{
			return new NullPostingIterator( termstr.c_str());
		}
		return new PostingIterator( this, m_database.get(), typeno, termno, termstr.c_str(), length, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term posting search index iterator: %s"), *m_errorhnd, 0);
}

PostingIteratorInterface*
	StorageClient::createFieldPostingIterator(
		const std::string& meta_fieldStart,
		const std::string& meta_fieldEnd) const
{
	try
	{
		return new MetaDataRangePostingIterator(
				new MetaDataReader( m_metaDataBlockCache, &m_metadescr, m_errorhnd),
				m_nof_documents.value(), meta_fieldStart, meta_fieldEnd, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating field posting iterator defined by meta data: %s"), *m_errorhnd, 0);
}

PostingIteratorInterface*
	StorageClient::createBrowsePostingIterator(
		const MetaDataRestrictionInterface* restriction,
		const Index& maxpos) const
{
	try
	{
		if (maxpos == 0)
		{
			if (restriction) delete restriction;
			return new NullPostingIterator("?");
		}
		else
		{
			return new BrowsePostingIterator( restriction, m_next_docno.value()-1, maxpos);
		}
	}
	catch (const std::bad_alloc& err)
	{
		m_errorhnd->report( _TXT("out of memory creating browse posting iterator"));
	}
	catch (const std::runtime_error& err)
	{
		m_errorhnd->report( _TXT("error creating browse posting iterator: %s"), err.what());
	}
	if (restriction) delete restriction;
	return 0;
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

AclReaderInterface* StorageClient::createAclReader() const
{
	try
	{
		return new AclReader( this, m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating ACL reader: %s"), *m_errorhnd, 0);
}

StorageTransactionInterface*
	StorageClient::createTransaction()
{
	try
	{
		if (m_statisticsProc)
		{
			TransactionLock lock( this);
			if (!m_statisticsBuilder.get())
			{
				m_statisticsBuilder.reset( m_statisticsProc->createBuilder());
			}
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
	virtual Index getOrCreate( const std::string& name)
	{
		return m_storage->allocTypenoImm( name);
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
	virtual Index getOrCreate( const std::string& name)
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
	virtual Index getOrCreate( const std::string& name)
	{
		if (!m_storage->withAcl())
		{
			throw strus::runtime_error( "%s", _TXT( "storage configured without ACL. No users can be created"));
		}
		return m_storage->allocUsernoImm( name);
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
	virtual Index getOrCreate( const std::string& name)
	{
		return m_storage->allocAttribnoImm( name);
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

	virtual Index getOrCreate( const std::string& name)
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

Index StorageClient::allocTypenoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_TermType::ReadWriter stor(m_database.get());

	utils::ScopedLock lock( m_immalloc_typeno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_typeno.allocIncrement());
	}
	return rt;
}

Index StorageClient::allocUsernoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_UserName::ReadWriter stor( m_database.get());

	utils::ScopedLock lock( m_immalloc_userno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_userno.allocIncrement());
	}
	return rt;
}

Index StorageClient::allocAttribnoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_AttributeKey::ReadWriter stor( m_database.get());

	utils::ScopedLock lock( m_immalloc_attribno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_attribno.allocIncrement());
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
		Index typeno = getTermType( type);
		Index termno = getTermValue( term);
		if (typeno && termno)
		{
			return DatabaseAdapter_DocFrequency::get( m_database.get(), typeno, termno);
		}
		else
		{
			return 0;
		}
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

Index StorageClient::termTypeNumber( const std::string& type) const
{
	return getTermType( type);
}

bool StorageClient::isForwardIndexTerm( const std::string& type) const
{
	Index typeno = getTermType( type);
	if (!typeno) return false;
	return DatabaseAdapter_ForwardIndex::exists( m_database.get(), typeno);
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

MetaDataRestrictionInterface* StorageClient::createMetaDataRestriction() const
{
	try
	{
		return new MetaDataRestriction( this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating meta data restriction object: %s"), *m_errorhnd, 0);
}

void StorageClient::loadTermnoMap( const char* termnomap_source)
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	if (!transaction.get()) throw strus::runtime_error( "%s", _TXT("error loading termno map"));
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
		throw strus::runtime_error( _TXT( "failed to build termno map: %s"), err.what());
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

bool StorageClient::fetchNextStatisticsMessage( const void*& msg, std::size_t& msgsize)
{
	TransactionLock lock( this);
	return m_statisticsBuilder->fetchMessage( msg, msgsize);
}

StatisticsIteratorInterface* StorageClient::createAllStatisticsIterator( bool sign)
{
	try
	{
		if (!m_statisticsProc)
		{
			throw strus::runtime_error( "%s", _TXT( "no statistics message processor defined"));
		}
		{
			TransactionLock lock( this);
			if (!m_statisticsBuilder.get())
			{
				m_statisticsBuilder.reset( m_statisticsProc->createBuilder());
			}
		}
		return new StatisticsInitIterator( this, m_database.get(), sign, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating statistics message iterator: %s"), *m_errorhnd, 0);
}

StatisticsIteratorInterface* StorageClient::createChangeStatisticsIterator()
{
	try
	{
		if (!m_statisticsProc)
		{
			throw strus::runtime_error( "%s", _TXT( "no statistics message processor defined"));
		}
		{
			TransactionLock lock( this);
			if (!m_statisticsBuilder.get())
			{
				m_statisticsBuilder.reset( m_statisticsProc->createBuilder());
			}
		}
		return new StatisticsUpdateIterator( this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating statistics message iterator: %s"), *m_errorhnd, 0);
}

const StatisticsProcessorInterface*  StorageClient::getStatisticsProcessor() const
{
	return m_statisticsProc;
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
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData( key, value);
				break;
			}
			case strus::DatabaseKey::TermTypeInvPrefix:
			{
				strus::TermTypeInvData( key, value);
				break;
			}
			case strus::DatabaseKey::TermValueInvPrefix:
			{
				strus::TermValueInvData( key, value);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData( key, value);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::MetaDataDescription metadescr( database);
				strus::DocMetaDataData( &metadescr, key, value);
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
		std::string ks( extractKeyString( key));
		char buf[ 256];
		snprintf( buf, 256, _TXT( "error in checked key '%s':"), ks.c_str());
		errorlog << buf << err.what() << std::endl;
	}
}

bool StorageClient::checkStorage( std::ostream& errorlog) const
{
	try
	{
		strus::local_ptr<strus::DatabaseCursorInterface>
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

void StorageClient::close()
{
	try
	{
		storeVariables();
	}
	CATCH_ERROR_MAP( _TXT("error storing variables in close of storage client: %s"), *m_errorhnd);
	m_database->close();
	m_close_called = true;
}



