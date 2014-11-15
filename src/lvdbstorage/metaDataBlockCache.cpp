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
#include <stdint.h>

using namespace strus;

static uint32_t hashint( uint32_t a)
{
	a = a ^ (a>>4);
	a = (a^0xdeadbeef) + (a<<5);
	a = a ^ (a>>11);
	return a;
}

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
	if (blockno > MaxBlockno || blockno <= 0) throw std::runtime_error("block number out of range (MetaDataBlockCache)");
	std::size_t blkidx = blockno-1;
	std::size_t idx_level2 = blkidx % NodeSize;
	std::size_t idx_level1 = blkidx / NodeSize;

	boost::shared_ptr<BlockNodeArray> ar1 = m_ar[ aridx(varname)];
	if (!ar1.get()) return;
	
	// Level 1:
	boost::shared_ptr<BlockArray> ar2 = (*ar1)[ idx_level1];
	if (!ar2.get()) return;

	// Level 2:
	(*ar2)[ idx_level2].reset();
}


float MetaDataBlockCache::getValue( Index docno, char varname)
{
	if (docno > MaxDocno || docno <= 0) throw std::runtime_error("document number out of range (MetaDataBlockCache)");
	std::size_t docidx     = (std::size_t)(docno -1);
	std::size_t blkidx     = docidx % MetaDataBlock::MetaDataBlockSize;
	std::size_t idx_level2 = blkidx % NodeSize;
	std::size_t idx_level1 = blkidx / NodeSize;

	// The fact that the reference counting of shared_ptr is
	// thread safe is used to implement some kind of RCU:
	boost::shared_ptr<BlockNodeArray> ar1 = m_ar[ aridx(varname)];
	while (!ar1.get())
	{
		m_ar[ aridx(varname)].reset( new BlockNodeArray());
		ar1 = m_ar[ aridx(varname)];
	}
	
	// Level 1:
	boost::shared_ptr<BlockArray>& ar2 = (*ar1)[ idx_level1];
	if (!ar2.get())
	{
		unsigned int midx = hashint( idx_level1) % NofMutexLevel1;
		m_mutex_level1[ midx].lock();

		try
		{
			ar2 = (*ar1)[ idx_level1];
			if (!ar2.get())
			{
				(*ar1)[ idx_level1].reset( new BlockArray());
				ar2 = (*ar1)[ idx_level1];
			}
		}
		catch (const std::bad_alloc&)
		{
			m_mutex_level1[ midx].unlock();
			throw std::bad_alloc();
		}
		m_mutex_level1[ midx].unlock();
	}

	// Level 2:
	boost::shared_ptr<MetaDataBlock>& blkref = (*ar2)[ idx_level2];
	if (!blkref.get())
	{
		blkref.reset( MetaDataReader::readBlockFromDB( 
				m_db, MetaDataBlock::blockno( docno), varname));
	}
	return blkref->getValue( docno);
}

