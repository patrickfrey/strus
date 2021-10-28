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
#include "strus/lib/error.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/unordered_map.hpp"
#include "strus/base/configParser.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/databaseClientUndefinedStub.hpp"
#include "databaseKey.hpp"
#include "byteOrderMark.hpp"
#include "storageTransaction.hpp"
#include "storageDocumentChecker.hpp"
#include "extractKeyValueData.hpp"
#include "documentFrequencyCache.hpp"
#include "metaDataBlockCache.hpp"
#include "metaDataRestriction.hpp"
#include "metaDataReader.hpp"
#include "postingIterator.hpp"
#include "ffPostingIterator.hpp"
#include "structIterator.hpp"
#include "browsePostingIterator.hpp"
#include "nullPostingIterator.hpp"
#include "databaseAdapter.hpp"
#include "forwardIterator.hpp"
#include "documentTermIterator.hpp"
#include "indexPacker.hpp"
#include "attributeReader.hpp"
#include "keyAllocatorInterface.hpp"
#include "valueIterator.hpp"
#include "aclReader.hpp"
#include "storageDump.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

using namespace strus;

#define MODULENAME "storageClient"

static char const** getConfigParamList( const DatabaseInterface* db);

StorageClient::StorageClient(
		const DatabaseInterface* database_,
		const StatisticsProcessorInterface* statisticsProc_,
		const std::string& databaseConfig,
		ErrorBufferInterface* errorhnd_)
	:m_dbtype(database_)
	,m_database()
	,m_cfgparam(getConfigParamList(database_))
	,m_next_typeno(0)
	,m_next_termno(0)
	,m_next_structno(0)
	,m_next_docno(0)
	,m_next_userno(0)
	,m_next_attribno(0)
	,m_nof_documents(0)
	,m_metaDataBlockCache()
	,m_documentFrequencyCache()
	,m_close_called(false)
	,m_statisticsProc(statisticsProc_)
	,m_statisticsPath()
	,m_errorhnd(errorhnd_)
{
	init( databaseConfig);
}

StorageClient::~StorageClient()
{
	if (!m_close_called) try
	{
		storeVariables();
	}
	CATCH_ERROR_MAP( _TXT("error closing storage client: %s"), *m_errorhnd);
	std::free( m_cfgparam);
}

static char const** getConfigParamList( const DatabaseInterface* db)
{
	char const** rt;
	std::vector<const char*> cfgar;
	char const** cfg = db->getConfigParameters();
	for (int ci = 0; cfg[ci]; ++ci) cfgar.push_back( cfg[ci]);
	cfgar.push_back( "acl");
	cfgar.push_back( "statsproc");
	cfgar.push_back( "database");
	rt = (char const**)std::malloc( (cfgar.size()+1) * sizeof(rt[0]));
	if (rt == NULL) throw std::bad_alloc();
	std::memcpy( rt, cfgar.data(), cfgar.size() * sizeof(rt[0]));
	rt[ cfgar.size()] = 0;
	return rt;
}

void StorageClient::init( const std::string& databaseConfig)
{
	std::string databaseConfigCopy( databaseConfig);
	removeKeyFromConfigString( databaseConfigCopy, "acl", m_errorhnd);
	removeKeyFromConfigString( databaseConfigCopy, "statsproc", m_errorhnd);

	Reference<DatabaseClientInterface> db( m_dbtype->createClient( databaseConfigCopy));
	if (!db.get()) throw strus::runtime_error(_TXT("failed to initialize database client: %s"), m_errorhnd->fetchError());

	m_database = db;
	m_close_called = false;
	if (m_statisticsProc)
	{
		if (!extractStringFromConfigString( m_statisticsPath, databaseConfigCopy, "path", m_errorhnd))
		{
			throw strus::runtime_error(_TXT("variable '%s' not defined in configuration"), "path");
		}
	}
	MetaDataDescription metadescr;
	metadescr.load( m_database.get());
	m_metaDataBlockCache.reset( new MetaDataBlockCache( m_database.get(), metadescr));

	loadVariables( m_database.get());
}

bool StorageClient::reload( const std::string& databaseConfig)
{
	try
	{
		std::string src = databaseConfig;
		std::string statsproc;
		bool useAcl;
		bool fillDfCache = (0!=m_documentFrequencyCache.get());

		if (extractBooleanFromConfigString( useAcl, src, "acl", m_errorhnd))
		{
			if (useAcl != withAcl())
			{
				if (useAcl == true)
				{
					m_next_userno.set( 1);
				}
				else
				{
					throw std::runtime_error(_TXT("'acl' cannot be switched off"));
				}
			}
		}
		(void)removeKeyFromConfigString( src, "database", m_errorhnd);

		MetaDataDescription metadescr;
		metadescr.load( m_database.get());

		if (m_errorhnd->hasError()) return false;

		close();
		m_next_typeno.set(0);
		m_next_termno.set(0);
		m_next_structno.set(0);
		m_next_docno.set(0);
		m_next_userno.set(0);
		m_next_attribno.set(0);
		m_nof_documents.set(0);

		m_database.reset( new DatabaseClientUndefinedStub( m_errorhnd));
		//... the assignment of DatabaseClientUndefinedStub guarantees that m_database is initialized, event if 'init' throws
		m_metaDataBlockCache.reset( new MetaDataBlockCache( m_database.get(), metadescr));
		//... this assignment guarantees that m_metaDataBlockCache is initialized, event if 'init' throws

		m_documentFrequencyCache.reset();
		m_statisticsPath.clear();

		init( databaseConfig);
		if (fillDfCache) fillDocumentFrequencyCache();
		return true;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' reloading configuration: %s"), MODULENAME, *m_errorhnd, false);
}

long StorageClient::diskUsage() const
{
	return m_database->diskUsage();
}

BlockStatistics StorageClient::blockStatistics() const
{
	try
	{
		strus::local_ptr<strus::DatabaseCursorInterface>
			cursor( m_database->createCursor( strus::DatabaseOptions()));
		if (!cursor.get()) throw strus::runtime_error( _TXT("failed to create database cursor: %s"), m_errorhnd->fetchError());

		std::vector<BlockStatistics::Element> elements;
		std::map<char,int64_t> bcmap;
		std::map<const char*,int64_t> kemap;

		strus::DatabaseCursorInterface::Slice key = cursor->seekFirst( 0, 0);
		for (; key.defined(); key = cursor->seekNext())
		{
			if (key.size() == 0)
			{
				throw strus::runtime_error_ec( ErrorCodeDataCorruption, _TXT( "found empty key in storage"));
			}
			bcmap[ key.ptr()[0]] += key.size() + cursor->value().size();
			//... Sum of key value is not accurate, but we do not know how to get a value otherwise
		}
		std::map<char,int64_t>::const_iterator
			bi = bcmap.begin(), be = bcmap.end();
		for (; bi != be; ++bi)
		{
			const char* type = DatabaseKey::keyPrefixName( (DatabaseKey::KeyPrefix)bi->first);
			if (!type)
			{
				throw strus::runtime_error_ec( ErrorCodeDataCorruption, _TXT( "unknown block type found in storage: %c"), bi->first);
			}
			kemap[ type] = bi->second;
		}
		std::map<const char*,int64_t>::const_iterator
			ki = kemap.begin(), ke = kemap.end();
		for (; ki != ke; ++ki)
		{
			elements.push_back( BlockStatistics::Element( ki->first, ki->second));
		}
		return BlockStatistics( elements);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error listing block storage occupation statistics: %s"), *m_errorhnd, BlockStatistics());
}

std::string StorageClient::config() const
{
	try
	{
		std::string rt( m_database->config());
		strus::shared_ptr<MetaDataBlockCache> mt = m_metaDataBlockCache;

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
	strus::shared_ptr<MetaDataBlockCache> mt = m_metaDataBlockCache;
	// Refresh all entries touched by the inserts/updates written
	std::vector<Index>::const_iterator ri = refreshList.begin(), re = refreshList.end();
	for (; ri != re; ++ri)
	{
		mt->declareVoid( *ri);
	}
	mt->refresh();
}

void StorageClient::resetMetaDataBlockCache( const strus::shared_ptr<MetaDataBlockCache>& mdcache)
{
	m_metaDataBlockCache = mdcache;
}

static Index versionNo( Index major, Index minor)
{
	return (major * 1000) + minor;
}

void StorageClient::loadVariables( DatabaseClientInterface* database_)
{
	ByteOrderMark byteOrderMark;
	Index bom;
	ByteOrderMark storage_byteOrderMark;
	Index next_termno_;
	Index next_typeno_;
	Index next_structno_;
	Index next_docno_;
	Index next_attribno_;
	Index nof_documents_;
	Index next_userno_;
	Index version_;

	DatabaseAdapter_Variable<Index>::Reader varstor( database_);
	if (!varstor.load( "TermNo", next_termno_)
	||  !varstor.load( "TypeNo", next_typeno_)
	||  !varstor.load( "DocNo", next_docno_)
	||  !varstor.load( "AttribNo", next_attribno_)
	||  !varstor.load( "NofDocs", nof_documents_))
	{
		throw strus::runtime_error( "%s",  _TXT( "corrupt storage, not all mandatory variables defined"));
	}
	if (!varstor.load( "StructNo", next_structno_))
	{
		next_structno_ = 1;
	}
	if (!varstor.load( "Version", version_))
	{
		version_ = versionNo( 0, 4);
	}
	if (version_ / 1000 != STRUS_STORAGE_VERSION_MAJOR || version_ / 1000 > STRUS_STORAGE_VERSION_MINOR)
	{
		unsigned int major = version_ / 1000;
		unsigned int minor = version_ % 1000;
		throw strus::runtime_error( _TXT( "incompatible storage version %u.%u software is %u.%u."), major, minor, (unsigned int)STRUS_STORAGE_VERSION_MAJOR, (unsigned int)STRUS_STORAGE_VERSION_MINOR);
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
	m_next_structno.set( next_structno_);
	m_next_docno.set( next_docno_);
	m_next_attribno.set( next_attribno_);
	m_nof_documents.set( nof_documents_);
	m_next_userno.set( next_userno_);
}

void StorageClient::storeVariables()
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	if (!transaction.get()) throw std::runtime_error( _TXT("error storing variables"));
	getVariablesWriteBatch( transaction.get(), 0);
	transaction->commit();
}

void StorageClient::getVariablesWriteBatch(
		DatabaseTransactionInterface* transaction,
		int nof_documents_incr)
{
	DatabaseAdapter_Variable<Index>::Writer varstor( m_database.get());
	varstor.store( transaction, "TermNo", m_next_termno.value());
	varstor.store( transaction, "TypeNo", m_next_typeno.value());
	varstor.store( transaction, "StructNo", m_next_structno.value());
	varstor.store( transaction, "DocNo", m_next_docno.value());
	varstor.store( transaction, "AttribNo", m_next_attribno.value());
	varstor.store( transaction, "NofDocs", m_nof_documents.value() + nof_documents_incr);
	if (withAcl())
	{
		varstor.store( transaction, "UserNo", m_next_userno.value());
	}
}

Index StorageClient::getTermValue( const std::string& name) const
{
	return DatabaseAdapter_TermValue::Reader( m_database.get()).get( name);
}

Index StorageClient::getTermType( const std::string& name) const
{
	return DatabaseAdapter_TermType::Reader( m_database.get()).get( string_conv::tolower( name));
}

Index StorageClient::getStructType( const std::string& name) const
{
	return DatabaseAdapter_StructType::Reader( m_database.get()).get( string_conv::tolower( name));
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
	return DatabaseAdapter_AttributeKey::Reader( m_database.get()).get( string_conv::tolower( name));
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
		const Index& length,
		const TermStatistics& stats) const
{
	try
	{
		Index typeno = getTermType( typestr);
		Index termno = getTermValue( termstr);
		if (!typeno || !termno)
		{
			return new NullPostingIterator( termstr.c_str());
		}
		return new PostingIterator( this, m_database.get(), typeno, termno, termstr.c_str(), length, stats.documentFrequency(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating term posting search index iterator: %s"), *m_errorhnd, 0);
}

PostingIteratorInterface*
	StorageClient::createFrequencyPostingIterator(
		const std::string& typestr,
		const std::string& termstr,
		const TermStatistics& stats) const
{
	try
	{
		Index typeno = getTermType( typestr);
		Index termno = getTermValue( termstr);

		if (!typeno || !termno)
		{
			return new NullPostingIterator( termstr.c_str());
		}
		else
		{
			double df = documentFrequency( typeno, termno);
			double N = nofDocumentsInserted();
			if (df / N > Constants::stopwordDfFactor())
			{
				return new FfPostingIterator( this, m_database.get(), typeno, termno, termstr.c_str(), stats.documentFrequency(), m_errorhnd);
			}
			else
			{
				return new FfNoIndexSetPostingIterator( this, m_database.get(), typeno, termno, termstr.c_str(), stats.documentFrequency(), m_errorhnd);
			}
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating ff posting search index iterator: %s"), *m_errorhnd, 0);
}

StructureIteratorInterface*
	StorageClient::createStructureIterator() const
{
	try
	{
		return new StructIterator( this, m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating structure iterator: %s"), *m_errorhnd, 0);
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
			return new NullPostingIterator("?");
		}
		else
		{
			return new BrowsePostingIterator( restriction, m_next_docno.value()-1, maxpos);
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating browse posting iterator: %s"), *m_errorhnd, 0);
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
	InvertedAclIterator( const DatabaseClientInterface* database_, strus::Index userno_, ErrorBufferInterface* errorhnd_)
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
		return new AclReader( this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating ACL reader: %s"), *m_errorhnd, 0);
}

StorageTransactionInterface*
	StorageClient::createTransaction()
{
	try
	{
		return new StorageTransaction( this, m_next_typeno.value(), m_errorhnd);
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
		return new StorageDocumentChecker( this, docid, logfilename, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document checker: %s"), *m_errorhnd, 0);
}

void StorageClient::declareNofDocumentsInserted( int incr)
{
	m_nof_documents.increment( incr);
}

class TypenoAllocator
	:public KeyAllocatorInterface
{
public:
	explicit TypenoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name)
	{
		return m_storage->allocTypenoImm( name);
	}
	virtual Index alloc()
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for non immediate alloc"), "typeno");
	}

private:
	StorageClient* m_storage;
};

class StructnoAllocator
	:public KeyAllocatorInterface
{
public:
	explicit StructnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name)
	{
		return m_storage->allocStructnoImm( name);
	}
	virtual Index alloc()
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for non immediate alloc"), "structno");
	}

private:
	StorageClient* m_storage;
};

class DocnoAllocator
	:public KeyAllocatorInterface
{
public:
	explicit DocnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(false),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name)
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for immediate alloc"), "docno");
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
	explicit UsernoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name)
	{
		if (!m_storage->withAcl())
		{
			throw std::runtime_error( _TXT( "storage configured without ACL. No users can be created"));
		}
		return m_storage->allocUsernoImm( name);
	}
	virtual Index alloc()
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for non immediate alloc"), "userno");
	}
private:
	StorageClient* m_storage;
};

class AttribnoAllocator
	:public KeyAllocatorInterface
{
public:
	explicit AttribnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(true),m_storage(storage_){}
	virtual Index getOrCreate( const std::string& name)
	{
		return m_storage->allocAttribnoImm( name);
	}
	virtual Index alloc()
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for non immediate alloc"), "attribno");
	}
private:
	StorageClient* m_storage;
};

class TermnoAllocator
	:public KeyAllocatorInterface
{
public:
	explicit TermnoAllocator( StorageClient* storage_)
		:KeyAllocatorInterface(false),m_storage(storage_){}

	virtual Index getOrCreate( const std::string& name)
	{
		throw strus::runtime_error( _TXT( "cannot use %s allocator for immediate alloc"), "termno");
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

KeyAllocatorInterface* StorageClient::createStructnoAllocator()
{
	return new StructnoAllocator( this);
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

	strus::scoped_lock lock( m_immalloc_typeno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_typeno.allocIncrement());
	}
	return rt;
}

Index StorageClient::allocStructnoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_StructType::ReadWriter stor(m_database.get());

	strus::scoped_lock lock( m_immalloc_structno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_structno.allocIncrement());
	}
	return rt;
}

Index StorageClient::allocUsernoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_UserName::ReadWriter stor( m_database.get());

	strus::scoped_lock lock( m_immalloc_userno_mutex);
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

	strus::scoped_lock lock( m_immalloc_attribno_mutex);
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

Index StorageClient::maxStructTypeNo() const
{
	return m_next_structno.value() -1;
}

IndexSetIterator StorageClient::getAclIterator( strus::Index docno) const
{
	return IndexSetIterator( m_database.get(), DatabaseKey::AclBlockPrefix, BlockKey(docno), false);
}

IndexSetIterator StorageClient::getUserAclIterator( strus::Index userno) const
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

Index StorageClient::documentFrequency( strus::Index typeno, strus::Index termno) const
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

Index StorageClient::structTypeNumber( const std::string& structname) const
{
	return getStructType( structname);
}

Index StorageClient::termTypeNumber( const std::string& type) const
{
	return getTermType( type);
}

Index StorageClient::termValueNumber( const std::string& value) const
{
	return getTermValue( value);
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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating %s iterator: %s"), "term type", *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createStructTypeIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_StructType>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating %s iterator: %s"), "struct type", *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createTermValueIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_TermValue>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating %s iterator: %s"), "term value", *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createDocIdIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_DocId>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating %s iterator: %s"), "document id", *m_errorhnd, 0);
}

ValueIteratorInterface* StorageClient::createUserNameIterator() const
{
	try
	{
		return new ValueIterator<DatabaseAdapter_UserName>( m_database.get(), m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating %s iterator: %s"), "user name", *m_errorhnd, 0);
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
		return new MetaDataReader( m_metaDataBlockCache, m_errorhnd);
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
	if (!transaction.get()) throw std::runtime_error( _TXT("error loading termno map"));
	strus::unordered_map<std::string,Index> termno_map;
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

std::vector<StatisticsMessage> StorageClient::loadAllStatisticsMessages() const
{
	try
	{
		if (!m_statisticsProc)
		{
			throw std::runtime_error( _TXT( "no statistics message processor defined"));
		}
		if (!m_statisticsProc)
		{
			throw strus::runtime_error(_TXT("error no statistics processor defined"));
		}
		strus::Reference<StatisticsBuilderInterface> builder( m_statisticsProc->createBuilder( ""/*path*/));
		if (!builder.get())
		{
			throw strus::runtime_error(_TXT("failed to create statistics builder: %s"), m_errorhnd->fetchError());
		}
		int nofdocs = nofDocumentsInserted();
		builder->addNofDocumentsInsertedChange( nofdocs);

		std::map<Index,std::size_t> typenomap;
		std::map<Index,std::size_t> termnomap;
		std::string strings;

		// Fill a map with the strings of all types in the collection:
		{
			DatabaseAdapter_TermType::Cursor typecursor( m_database.get());
			Index typeno;
			std::string typestr;
			for (bool more=typecursor.loadFirst( typestr, typeno); more; more=typecursor.loadNext( typestr, typeno))
			{
				typenomap[ typeno] = strings.size();
				strings.append( typestr);
				strings.push_back( '\0');
			}
		}
		// Fill a map with the strings of all terms in the collection:
		{
			DatabaseAdapter_TermValue::Cursor termcursor( m_database.get());
			Index termno;
			std::string termstr;
			for (bool more=termcursor.loadFirst( termstr, termno); more; more=termcursor.loadNext( termstr, termno))
			{
				termnomap[ termno] = strings.size();
				strings.append( termstr);
				strings.push_back( '\0');
			}
		}
		// Feed all df changes to the statistics message builder:
		{
			DatabaseAdapter_DocFrequency::Cursor dfcursor( m_database.get());
			Index typeno;
			Index termno;
			Index df;

			for (bool more=dfcursor.loadFirst( typeno, termno, df); more; more=dfcursor.loadNext( typeno, termno, df))
			{
				std::map<Index,std::size_t>::const_iterator ti;
				ti = typenomap.find( typeno);
				if (ti == typenomap.end()) throw strus::runtime_error( _TXT( "encountered undefined type (no %d) when populating df's"), (int)typeno);
				const char* typenam = strings.c_str() + ti->second;

				ti = termnomap.find( termno);
				if (ti == termnomap.end()) throw strus::runtime_error( "%s",  _TXT( "encountered undefined term when populating df's"));
				const char* termnam = strings.c_str() + ti->second;

				builder->addDfChange( typenam, termnam, df);
			}
		}
		return builder->getMessages();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading all statistics messages: %s"), *m_errorhnd, std::vector<StatisticsMessage>());
}

StatisticsIteratorInterface* StorageClient::createChangeStatisticsIterator( const TimeStamp& timestamp) const
{
	try
	{
		if (!m_statisticsProc)
		{
			throw std::runtime_error( _TXT( "no statistics message processor defined"));
		}
		return m_statisticsProc->createIterator( m_statisticsPath, timestamp);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating statistics message iterator: %s"), *m_errorhnd, 0);
}

std::vector<TimeStamp> StorageClient::getChangeStatisticTimeStamps() const
{
	try
	{
		if (!m_statisticsProc)
		{
			throw std::runtime_error( _TXT( "no statistics message processor defined"));
		}
		return m_statisticsProc->getChangeTimeStamps( m_statisticsPath);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating list of statistics message timestamps: %s"), *m_errorhnd, std::vector<TimeStamp>());
}

StatisticsMessage StorageClient::loadChangeStatisticsMessage( const TimeStamp& timestamp) const
{
	try
	{
		if (!m_statisticsProc)
		{
			throw std::runtime_error( _TXT( "no statistics message processor defined"));
		}
		return m_statisticsProc->loadChangeMessage( m_statisticsPath, timestamp);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading a change statistics message: %s"), *m_errorhnd, StatisticsMessage());
}

const StatisticsProcessorInterface*  StorageClient::getStatisticsProcessor() const
{
	return m_statisticsProc;
}

struct CheckHistory
{
	CheckHistory()
		:elemid(0),prevKey(0){}

	strus::Index elemid;
	char prevKey;
};

static void checkKeyValue(
		const strus::StorageClient* storage,
		const strus::DatabaseClientInterface* database,
		const strus::DatabaseCursorInterface::Slice& key,
		const strus::DatabaseCursorInterface::Slice& value,
		CheckHistory& history,
		std::ostream& errorlog)
{
	try
	{
		if (key.ptr()[0] != history.prevKey)
		{
			history.elemid = 0;
			history.prevKey = key.ptr()[0];
		}
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
			case strus::DatabaseKey::StructTypePrefix:
			{
				strus::TermTypeData( key, value);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData( key, value);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData( key, value);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData( key, value);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData( key, value);
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
			case strus::DatabaseKey::StructTypeInvPrefix:
			{
				strus::TermTypeInvData( key, value);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData( key, value);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData data( key, value);
				if (data.posinfo.empty() || history.elemid >= data.posinfo[0].docno)
				{
					throw strus::runtime_error(_TXT("corrupt index: empty or overlapping %s blocks"), "posinfo");
				}
				history.elemid = data.docno;
				break;
			}
			case strus::DatabaseKey::FfBlockPrefix:
			{
				strus::FfBlockData data( key, value);
				if (data.postings.empty() || history.elemid >= data.postings[0].docno)
				{
					throw strus::runtime_error(_TXT("corrupt index: empty or overlapping %s blocks"), "ff");
				}
				history.elemid = data.docno;
				break;
			}
			case strus::DatabaseKey::StructBlockPrefix:
			{
				strus::StructBlockData data( key, value);
				if (data.structures.empty())
				{
					throw strus::runtime_error(_TXT("corrupt index: empty %s block"), "structure");
				}
				else
				{
					std::vector<strus::StructBlockData::Structure>::const_iterator
						di = data.structures.begin(), de = data.structures.end();
					for (; di != de; ++di)
					{
						if (di->structno <= 0 || di->structno > storage->maxStructTypeNo())
						{
							throw strus::runtime_error(_TXT("corrupt index: unknown or bad id of %s"), "structure");
						}
						if (di->source.start() <= 0 || di->source.end() <= 0)
						{
							throw strus::runtime_error(_TXT("corrupt index: bad header of %s"), "structure");
						}
						if (di->sink.start() <= 0 || di->sink.end() <= 0)
						{
							throw strus::runtime_error(_TXT("corrupt index: bad content of %s"), "structure");
						}
					}
				}
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData( key, value);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData data( key, value);
				if (data.docrangelist.empty() || history.elemid >= data.docrangelist[0].first)
				{
					throw strus::runtime_error(_TXT("corrupt index: empty or overlapping %s blocks"), "boolean document acl");
				}
				history.elemid = data.docno;
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData data( key, value);
				if (data.userrangelist.empty() || history.elemid >= data.userrangelist[0].first)
				{
					throw strus::runtime_error(_TXT("corrupt index: empty or overlapping %s blocks"), "boolean user acl");
				}
				history.elemid = data.userno;
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData data( key, value);
				if (data.docrangelist.empty() || history.elemid >= data.docrangelist[0].first)
				{
					throw strus::runtime_error(_TXT("corrupt index: empty or overlapping %s blocks"), "boolean document");
				}
				history.elemid = data.docno;
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
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData( key, value);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData( key, value);
				break;
			}
			default:
				throw strus::runtime_error(_TXT("unknown database key prefix: %c"), key.ptr()[0]);
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

StorageDumpInterface* StorageClient::createDump(
		const std::string& keyprefix) const
{
	try
	{
		return new StorageDump( m_database, keyprefix, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage dump interface: %s"), *m_errorhnd, 0);
}

const char** StorageClient::getConfigParameters() const
{
	return m_cfgparam;
}

bool StorageClient::checkStorage( std::ostream& errorlog) const
{
	try
	{
		CheckHistory history;
		strus::local_ptr<strus::DatabaseCursorInterface>
			cursor( m_database->createCursor( strus::DatabaseOptions()));
		if (!cursor.get()) return false;

		strus::DatabaseCursorInterface::Slice key = cursor->seekFirst( 0, 0);

		for (; key.defined(); key = cursor->seekNext())
		{
			if (key.size() == 0)
			{
				m_errorhnd->report( ErrorCodeDataCorruption, _TXT( "found empty key in storage"));
				return false;
			}
			checkKeyValue( this, m_database.get(), key, cursor->value(), history, errorlog);
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error checking storage: %s"), *m_errorhnd, false);
}

void StorageClient::close()
{
	try
	{
		if (!m_close_called)
		{
			storeVariables();
			m_database->compactDatabase();
			m_database->close();
			m_close_called = true;
		}
	}
	CATCH_ERROR_MAP( _TXT("error storing variables in close of storage: %s"), *m_errorhnd);
}

void StorageClient::compaction()
{
	try
	{
		storeVariables();
		m_database->compactDatabase();
	}
	CATCH_ERROR_MAP( _TXT("error in compaction of storage: %s"), *m_errorhnd);
}


