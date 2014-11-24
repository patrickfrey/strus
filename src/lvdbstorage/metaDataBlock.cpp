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

MetaDataBlock::MetaDataBlock( const MetaDataDescription* descr_, unsigned int blockno_)
	:m_descr(descr_),m_blockno(blockno_),m_ptr( std::calloc( MetaDataBlockSize, descr_->bytesize()))
{
	if (!m_ptr) throw std::bad_alloc();
}

MetaDataBlock::MetaDataBlock( const MetaDataDescription* descr_, 
				unsigned int blockno_,
				const char* blk_,
				std::size_t blksize_)
	:m_descr(descr_),m_blockno(blockno_),m_ptr( std::malloc( MetaDataBlockSize * descr_->bytesize()))
{
	std::size_t blkbytesize = m_descr->bytesize() * MetaDataBlockSize;
	if (blksize_ != blkbytesize) throw std::runtime_error( "meta data block size mismatch");
	std::memcpy( m_ptr, blk_, blkbytesize);
}

MetaDataBlock::MetaDataBlock( const MetaDataBlock& o)
	:m_descr(o.m_descr),m_blockno(o.m_blockno),m_ptr( std::malloc( MetaDataBlockSize * o.m_descr->bytesize()))
{
	std::size_t blkbytesize = m_descr->bytesize() * MetaDataBlockSize;
	std::memcpy( m_ptr, o.m_ptr, blkbytesize);
}

MetaDataBlock::~MetaDataBlock()
{
	std::free( m_ptr);
}

const MetaDataRecord MetaDataBlock::operator[]( std::size_t idx) const
{
	if (idx >= MetaDataBlockSize) throw std::logic_error( "array bound read in meta data block");
	void* recaddr = (char*)m_ptr + (idx * m_descr->bytesize());
	return MetaDataRecord( m_descr, recaddr);
}


