/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataBlockCache.hpp"
#include "strus/databaseClientInterface.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
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
	if (blockno > CacheSize || blockno <= 0) throw strus::runtime_error( _TXT( "block number out of range (meta data block cache)"));
	std::size_t blkidx = blockno-1;

	m_ar[ blkidx].reset();
}


const MetaDataRecord MetaDataBlockCache::get( const Index& docno)
{
	if (docno > MaxDocno || docno <= 0) throw strus::runtime_error( _TXT( "document number out of range (meta data block cache)"));
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

