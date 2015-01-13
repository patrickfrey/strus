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
#include "metaDataBlockMap.hpp"
#include "metaDataBlockCache.hpp"
#include "dataBlock.hpp"
#include "dataBlockStorage.hpp"
#include <leveldb/write_batch.h>
#include <cstring>
#include <boost/scoped_ptr.hpp>

using namespace strus;

MetaDataBlockMap::~MetaDataBlockMap()
{
}

void MetaDataBlockMap::deleteMetaData( Index docno)
{
	std::size_t ii=0, nn=m_descr->nofElements();
	for (; ii<nn; ++ii)
	{
		MetaDataKey key( docno, ii);
		m_map[ key] = ArithmeticVariant();
	}
}

void MetaDataBlockMap::deleteMetaData( Index docno, const std::string& varname)
{
	MetaDataKey key( docno, m_descr->getHandle( varname));
	m_map[ key] = ArithmeticVariant();
}

void MetaDataBlockMap::defineMetaData( Index docno, const std::string& varname, const ArithmeticVariant& value)
{
	MetaDataKey key( docno, m_descr->getHandle( varname));
	m_map[ key] = value;
}

void MetaDataBlockMap::getWriteBatch( leveldb::WriteBatch& batch, std::vector<Index>& cacheRefreshList)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	Index blockno = 0;
	boost::scoped_ptr<MetaDataBlock> blk;
	DataBlockStorage storage( m_db, DatabaseKey( DatabaseKey::DocMetaDataPrefix), false);

	for (; mi != me; ++mi)
	{
		Index docno = mi->first.first;
		Index elemhandle = mi->first.second;

		Index bn = MetaDataBlock::blockno( docno);
		if (bn != blockno)
		{
			if (blk.get())
			{
				storage.store( 
					DataBlock( DatabaseKey::DocMetaDataPrefix,
							blockno, blk->charptr(), blk->bytesize()), batch);
			}
			const DataBlock* mv;
			if (bn == blockno + 1 && blockno != 0)
			{
				mv = storage.loadNext();
			}
			else
			{
				mv = storage.load( bn);
			}
			blockno = bn;

			if (mv && mv->id() == blockno)
			{
				blk.reset( new MetaDataBlock( m_descr, blockno, mv->charptr(), mv->size()));
				cacheRefreshList.push_back( blockno);
			}
			else
			{
				blk.reset( new MetaDataBlock( m_descr, blockno));
			}
		}
		const MetaDataElement* elem = m_descr->get( elemhandle);
		MetaDataRecord record = (*blk)[ MetaDataBlock::index( docno)];
		record.setValue( elem, mi->second);
	}
	if (blk.get())
	{
		storage.store( 
			DataBlock( DatabaseKey::DocMetaDataPrefix,
					blockno, blk->charptr(), blk->bytesize()), batch);
	}
}


void MetaDataBlockMap::rewriteMetaData(
		const MetaDataDescription::TranslationMap& trmap,
		const MetaDataDescription& newDescr,
		leveldb::WriteBatch& batch)
{
	DataBlockStorage storage( m_db, DatabaseKey( DatabaseKey::DocMetaDataPrefix), false);
	const DataBlock* blk = storage.loadFirst();

	for (; blk != 0; blk = storage.loadNext())
	{
		MetaDataBlock oldblk( m_descr, blk->id(), blk->charptr(), blk->size());

		std::size_t newblk_bytesize = MetaDataBlock::BlockSize * newDescr.bytesize();
		char* newblk_data = (char*)std::calloc( MetaDataBlock::BlockSize, newDescr.bytesize());

		try
		{
			MetaDataRecord::translateBlock(
					trmap, newDescr, newblk_data,
					*m_descr, blk->ptr(), MetaDataBlock::BlockSize);
		}
		catch (const std::runtime_error& err)
		{
			std::free( newblk_data);
			throw err;
		}
		DataBlock newblk( DatabaseKey::DocMetaDataPrefix, blk->id(), newblk_data, newblk_bytesize);
		storage.store( newblk, batch);
	}
}



