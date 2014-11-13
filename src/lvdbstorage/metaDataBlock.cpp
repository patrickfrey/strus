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
#include "metaDataBlock.hpp"
#include "databaseKey.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

MetaDataBlock::MetaDataBlock( Index blockno_)
	:m_blockno(blockno_)
{
	std::memset( m_blk, 0, sizeof(m_blk));
}

MetaDataBlock::MetaDataBlock( Index blockno_,
				const float* blk_, std::size_t blksize_)
	:m_blockno(blockno_)
{
	if (blksize_ != MetaDataBlockSize) throw std::runtime_error( "meta data block size mismatch");
	std::memcpy( m_blk, blk_, sizeof(m_blk));
}

MetaDataBlock::MetaDataBlock( const MetaDataBlock& o)
	:m_blockno(o.m_blockno)
{
	std::memcpy( m_blk, o.m_blk, sizeof(m_blk));
}

void MetaDataBlock::setValue( Index docno, float value)
{
	if (blockno(docno) != m_blockno)
	{
		throw std::runtime_error( "internal: illegal write access on block (wrong block number)");
	}
	m_blk[ index(docno)] = value;
}

float MetaDataBlock::getValue( Index docno) const
{
	if (blockno( docno) != m_blockno)
	{
		throw std::runtime_error( "internal: illegal read access on block (wrong block number)");
	}
	return m_blk[ index(docno)];
}





