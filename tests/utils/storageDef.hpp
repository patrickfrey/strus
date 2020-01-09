/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Storage helpers for tests
/// \file "storageDef.hpp"
#ifndef _STRUS_CORE_TEST_STORAGE_DEF_HPP_INCLUDED
#define _STRUS_CORE_TEST_STORAGE_DEF_HPP_INCLUDED
#include "strus/lib/database_leveldb.hpp"
#include "strus/lib/storage.hpp"
#include "strus/reference.hpp"
#include "strus/index.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/storageMetaDataTableUpdateInterface.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>

namespace strus {
namespace test {

class StructureDef
{
public:
	StructureDef()
		:m_name(),m_header(),m_content(){}
	StructureDef( const std::string& name_, const IndexRange& header_, const IndexRange& content_)
		:m_name(name_),m_header(header_),m_content(content_){}
	StructureDef( const StructureDef& o)
		:m_name(o.m_name),m_header(o.m_header),m_content(o.m_content){}

	const std::string& name() const		{return m_name;}
	const IndexRange& header() const	{return m_header;}
	const IndexRange& content() const	{return m_content;}
	
private:
	std::string m_name;
	IndexRange m_header;
	IndexRange m_content;
};

class Storage
{
public:
	explicit Storage( const strus::FileLocatorInterface* fileLocator_, strus::ErrorBufferInterface* errorhnd_)
		:fileLocator(fileLocator_),errorhnd(errorhnd_),dbi(),sti(),sci(){}
	Storage( const Storage& o)
		:fileLocator(o.fileLocator),errorhnd(o.errorhnd),dbi(o.dbi),sti(o.sti),sci(o.sci){}
	~Storage(){}

	const strus::FileLocatorInterface* fileLocator;
	strus::ErrorBufferInterface* errorhnd;
	strus::Reference<strus::DatabaseInterface> dbi;
	strus::Reference<strus::StorageInterface> sti;
	strus::Reference<strus::StorageClientInterface> sci;

	void open( const char* config, bool reset)
	{
		dbi.reset( strus::createDatabaseType_leveldb( fileLocator, errorhnd));
		if (!dbi.get())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		sti.reset( strus::createStorageType_std( fileLocator, errorhnd));
		if (!sti.get() || errorhnd->hasError())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		if (reset)
		{
			(void)dbi->destroyDatabase( config);
			(void)errorhnd->fetchError();
		
			if (!sti->createStorage( config, dbi.get()))
			{
				throw std::runtime_error( errorhnd->fetchError());
			}
		}
		const strus::StatisticsProcessorInterface* statisticsMessageProc = 0;
		sci.reset( sti->createClient( config, dbi.get(), statisticsMessageProc));
		if (!sci.get())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
	}

	void close()
	{
		sci.reset();
		sti.reset();
		dbi.reset();
	}

	struct MetaDataDef
	{
		const char* key;
		const char* type;
	};

	void defineMetaData( MetaDataDef const* deflist)
	{
		strus::Reference<strus::StorageTransactionInterface> transaction( sci->createTransaction());
		if (!transaction.get()) throw strus::runtime_error( "failed to create transaction: %s", errorhnd->fetchError());
		strus::Reference<strus::StorageMetaDataTableUpdateInterface> update( transaction->createMetaDataTableUpdate());
		if (!update.get()) throw strus::runtime_error( "failed to create structure for declaring meta data: %s", errorhnd->fetchError());
		
		int di = 0;
		for (; deflist[di].key; ++di)
		{
			update->addElement( deflist[di].key, deflist[di].type);
		}
		update->done();
		if (!transaction->commit()) throw strus::runtime_error( "failed to commit meta data structure definition: %s", errorhnd->fetchError());
	}

	std::string dump()
	{
		strus::local_ptr<strus::StorageDumpInterface> chunkitr( sci->createDump( ""/*keyprefix*/));
	
		const char* chunk;
		std::size_t chunksize;
		std::string dumpcontent;
		while (chunkitr->nextChunk( chunk, chunksize))
		{
			dumpcontent.append( chunk, chunksize);
		}
		if (errorhnd->hasError())
		{
			throw std::runtime_error( errorhnd->fetchError());
		}
		return dumpcontent;
	}

	static void destroy( const char* config, const strus::FileLocatorInterface* fileLocator_, strus::ErrorBufferInterface* errorhnd_)
	{
		strus::local_ptr<strus::DatabaseInterface> dbi;
		dbi.reset( strus::createDatabaseType_leveldb( fileLocator_, errorhnd_));
		if (!dbi.get())
		{
			throw std::runtime_error( errorhnd_->fetchError());
		}
		dbi->destroyDatabase( config);
	}
};


struct Feature
{
	enum Kind
	{
		SearchIndex,
		ForwardIndex,
		Attribute,
		MetaData
	};
	static const char* kindName( Kind kind)
	{
		static const char* ar[] = {"searchindex","forwardindex","attribute","metadata"};
		return ar[ kind];
	}
	Kind kind;
	std::string type;
	std::string value;
	strus::Index pos;

	Feature()
		:kind(Attribute),type(),value(),pos(0){}
	Feature( Kind kind_, const std::string& type_, const std::string& value_, strus::Index pos_=0)
		:kind(kind_),type(type_),value(value_),pos(pos_){}
	Feature( const Feature& o)
		:kind(o.kind),type(o.type),value(o.value),pos(o.pos){}
	Feature& operator=( const Feature& o)
		{kind=o.kind;type=o.type;value=o.value;pos=o.pos; return *this;}

	bool operator <( const Feature& o) const	{return compare(o) < 0;}
	bool operator >( const Feature& o) const	{return compare(o) > 0;}
	bool operator <=( const Feature& o) const	{return compare(o) <= 0;}
	bool operator >=( const Feature& o) const	{return compare(o) >= 0;}
	bool operator ==( const Feature& o) const	{return compare(o) == 0;}
	bool operator !=( const Feature& o) const	{return compare(o) != 0;}

private:
	int compare( const Feature& o) const
	{
		if (kind != o.kind) return (int)kind - (int)o.kind;
		if (pos != o.pos) return (int)pos - (int)o.pos;
		if (type != o.type) return std::strcmp( type.c_str(), o.type.c_str());
		if (value != o.value) return std::strcmp( value.c_str(), o.value.c_str());
		return 0;
	}
};

}}//namespace
#endif

