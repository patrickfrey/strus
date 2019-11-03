/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataMap.hpp"
#include "metaDataBlockCache.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "dataBlock.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"
#include "private/internationalization.hpp"
#include <cstring>

using namespace strus;

MetaDataMap::~MetaDataMap()
{
}

void MetaDataMap::renameNewDocNumbers( const std::map<Index,Index>& renamemap)
{
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index docno = MetaDataKey(mi->first).first;
		if (KeyMap::isUnknown( docno))
		{
			Index elemno = MetaDataKey(mi->first).second;
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "metadata map");
			}
			MetaDataKey newkey( ri->second, elemno);
			m_map[ newkey] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void MetaDataMap::deleteMetaData( Index docno)
{
	std::size_t ii=0, nn=m_descr->nofElements();
	for (; ii<nn; ++ii)
	{
		MetaDataKey key( docno, ii);
		m_map[ key] = NumericVariant();
	}
}

void MetaDataMap::deleteMetaData( Index docno, const std::string& varname)
{
	Index elementhnd = m_descr->getHandle( varname);
	if (elementhnd < 0) throw strus::runtime_error(_TXT("delete unknown meta data element '%s'"), varname.c_str());
	MetaDataKey key( docno, elementhnd);
	m_map[ key] = NumericVariant();
}

void MetaDataMap::defineMetaData( Index docno, const std::string& varname, const NumericVariant& value)
{
	Index elementhnd = m_descr->getHandle( varname);
	if (elementhnd < 0) throw strus::runtime_error(_TXT("define unknown meta data element '%s'"), varname.c_str());
	MetaDataKey key( docno, elementhnd);
	m_map[ key] = value;
}

void MetaDataMap::getWriteBatch( DatabaseTransactionInterface* transaction, std::vector<Index>& cacheRefreshList)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	Index blockno = 0;
	MetaDataBlock blk;
	DatabaseAdapter_DocMetaData dbadapter( m_database, m_descr);
	for (; mi != me; ++mi)
	{
		Index docno = mi->first.first;
		Index elemhandle = mi->first.second;

		Index bn = MetaDataBlock::blockno( docno);
		if (bn != blockno)
		{
			if (!blk.empty())
			{
				dbadapter.store( transaction, blk);
			}
			if (dbadapter.load( bn, blk))
			{
				cacheRefreshList.push_back( bn);
			}
			else
			{
				blk.init( m_descr, bn);
			}
			blockno = bn;
		}
		const MetaDataElement* elem = m_descr->get( elemhandle);
		MetaDataRecord record = blk[ MetaDataBlock::index( docno)];
		record.setValue( elem, mi->second);
	}
	if (!blk.empty())
	{
		dbadapter.store( transaction, blk);
	}
}

struct ScopedCharArray
{
	explicit ScopedCharArray( std::size_t size_)
		:size(size_)
	{
		data = (char*)std::calloc( size, sizeof(*data));
		if (!data) throw std::bad_alloc();
	}
	~ScopedCharArray()
	{
		std::free( data);
	}
	char* data;
	std::size_t size;
};

void MetaDataMap::rewriteMetaData(
		const MetaDataDescription::TranslationMap& trmap,
		const MetaDataDescription& newDescr,
		DatabaseTransactionInterface* transaction)
{
	MetaDataBlock blk;
	DatabaseAdapter_DocMetaData dbadapter( m_database, m_descr);

	for (bool more=dbadapter.loadFirst(blk); more; more=dbadapter.loadNext(blk))
	{
		ScopedCharArray blob( MetaDataBlock::BlockSize * newDescr.bytesize());
		MetaDataRecord::translateBlock(
				trmap, newDescr, blob.data,
				*m_descr, blk.ptr(), MetaDataBlock::BlockSize);
		MetaDataBlock newblk( &newDescr, blk.blockno(), blob.data, blob.size);
		dbadapter.store( transaction, newblk);
	}
}

void MetaDataMap::clear()
{
	m_map.clear();
}

void MetaDataMap::reset( const strus::shared_ptr<MetaDataBlockCache>& mdcache_)
{
	clear();
	m_metaDataBlockCache = mdcache_;
	m_descr = &m_metaDataBlockCache->descr();
	
}

