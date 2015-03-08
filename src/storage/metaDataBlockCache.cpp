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
#include "strus/databaseClientInterface.hpp"
#include "databaseAdapter.hpp"
#include <stdexcept>
#include <stdint.h>

using namespace strus;

MetaDataBlockCache::MetaDataBlockCache( DatabaseClientInterface* database_, const MetaDataDescription& descr_)
	:m_database(database_),m_descr(descr_),m_dbadapter(database_, &m_descr)
{}

void MetaDataBlockCache::declareVoid( const Index& blockno)
{
	m_voidar.push_back( blockno);
}

void MetaDataBlockCache::refresh()
{
	std::vector<unsigned int>::const_iterator vi = m_voidar.begin(), ve = m_voidar.end();
	for (; vi != ve; ++vi)
	{
		resetBlock( *vi);
	}
}

void MetaDataBlockCache::resetBlock( const Index& blockno)
{
	if (blockno > CacheSize || blockno <= 0) throw std::runtime_error("block number out of range (MetaDataBlockCache)");
	std::size_t blkidx = blockno-1;

	m_ar[ blkidx].reset();
}


const MetaDataRecord MetaDataBlockCache::get( const Index& docno)
{
	if (docno > MaxDocno || docno <= 0) throw std::runtime_error("document number out of range (MetaDataBlockCache)");
	std::size_t docidx     = (std::size_t)(docno -1);
	std::size_t blkidx     = docidx / MetaDataBlock::BlockSize;
	Index blockno          = blkidx+1;

	// The fact that the reference counting of shared_ptr is
	// thread safe is used to implement some kind of RCU:
	utils::SharedPtr<MetaDataBlock> blkref = m_ar[ blkidx];
	while (!blkref.get())
	{
		MetaDataBlock* newblk = m_dbadapter.loadPtr( blockno);
		if (newblk)
		{
			m_ar[ blkidx].reset( newblk);
		}
		else
		{
			m_ar[ blkidx].reset( new MetaDataBlock( &m_descr, blockno));
		}
		blkref = m_ar[ blkidx];
	}
	return (*blkref)[ MetaDataBlock::index( docno)];
}

