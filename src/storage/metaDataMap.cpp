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
#include "metaDataMap.hpp"
#include "metaDataBlockCache.hpp"
#include "private/utils.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "dataBlock.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"
#include <cstring>
#include "private/utils.hpp"

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
			MetaDataKey newkey( docno, elemno);
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
		m_map[ key] = ArithmeticVariant();
	}
}

void MetaDataMap::deleteMetaData( Index docno, const std::string& varname)
{
	MetaDataKey key( docno, m_descr->getHandle( varname));
	m_map[ key] = ArithmeticVariant();
}

void MetaDataMap::defineMetaData( Index docno, const std::string& varname, const ArithmeticVariant& value)
{
	MetaDataKey key( docno, m_descr->getHandle( varname));
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


void MetaDataMap::rewriteMetaData(
		const MetaDataDescription::TranslationMap& trmap,
		const MetaDataDescription& newDescr,
		DatabaseTransactionInterface* transaction)
{
	MetaDataBlock blk;
	DatabaseAdapter_DocMetaData dbadapter( m_database, m_descr);

	for (bool more=dbadapter.loadFirst(blk); more; more=dbadapter.loadNext(blk))
	{
		std::size_t newblk_bytesize = MetaDataBlock::BlockSize * newDescr.bytesize();
		char* newblk_data = new char[ MetaDataBlock::BlockSize * newDescr.bytesize()];
		utils::ScopedArray<char> newblk_data_mem( newblk_data);

		MetaDataRecord::translateBlock(
				trmap, newDescr, newblk_data,
				*m_descr, blk.ptr(), MetaDataBlock::BlockSize);
		MetaDataBlock newblk( &newDescr, blk.blockno(), newblk_data, newblk_bytesize);
		dbadapter.store( transaction, newblk);
	}
}



