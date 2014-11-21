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
#include "metaDataBlockCache.hpp"
#include "metaDataReader.hpp"
#include "statistics.hpp"
#include <stdint.h>

using namespace strus;

MetaDataBlockCache::MetaDataBlockCache( leveldb::DB* db_)
	:m_db(db_)
{}

void MetaDataBlockCache::declareVoid( unsigned int blockno, char varname)
{
	m_voidar.push_back( VoidRef( blockno, varname));
}

void MetaDataBlockCache::refresh()
{
	std::vector<VoidRef>::const_iterator vi = m_voidar.begin(), ve = m_voidar.end();
	for (; vi != ve; ++vi)
	{
		resetBlock( vi->blockno, vi->varname);
	}
}

void MetaDataBlockCache::resetBlock( unsigned int blockno, char varname)
{
	if (blockno > CacheSize || blockno <= 0) throw std::runtime_error("block number out of range (MetaDataBlockCache)");
	std::size_t blkidx = blockno-1;

	boost::shared_ptr<CacheStruct> cache = m_ar[ aridx(varname)];
	if (!cache.get()) return;
	
	// Level 2:
	(*cache)[ blkidx].reset();
}


float MetaDataBlockCache::getValue( Index docno, char varname)
{
	if (docno > MaxDocno || docno <= 0) throw std::runtime_error("document number out of range (MetaDataBlockCache)");
	std::size_t docidx     = (std::size_t)(docno -1);
	std::size_t blkidx     = docidx / MetaDataBlock::MetaDataBlockSize;

	// The fact that the reference counting of shared_ptr is
	// thread safe is used to implement some kind of RCU:
	boost::shared_ptr<CacheStruct> cache = m_ar[ aridx(varname)];
	while (!cache.get())
	{
		m_ar[ aridx(varname)].reset( new CacheStruct());
		cache = m_ar[ aridx(varname)];
	}
	
	boost::shared_ptr<MetaDataBlock> blkref = (*cache)[ blkidx];
	while (!blkref.get())
	{
		Statistics::increment( Statistics::MetaDataCacheMiss);
		(*cache)[ blkidx].reset( 
			MetaDataReader::readBlockFromDB( 
				m_db, MetaDataBlock::blockno( docno), varname));
		blkref = (*cache)[ blkidx];
	}
	return blkref->getValue( docno);
}

