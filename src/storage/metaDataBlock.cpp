/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataBlock.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

MetaDataBlock::MetaDataBlock()
	:m_descr(0),m_blockno(0),m_ptr(0)
{
}

MetaDataBlock::MetaDataBlock( const MetaDataDescription* descr_, const Index& blockno_)
	:m_descr(descr_),m_blockno(blockno_),m_ptr( std::calloc( BlockSize, descr_->bytesize()))
{
	if (!m_ptr) throw std::bad_alloc();
}

MetaDataBlock::MetaDataBlock( const MetaDataDescription* descr_, 
				const Index& blockno_,
				const char* blk_,
				std::size_t blksize_)
	:m_descr(descr_),m_blockno(blockno_),m_ptr(0)
{
	std::size_t blkbytesize = m_descr->bytesize() * BlockSize;
	if (blksize_ != blkbytesize) throw std::runtime_error( _TXT( "meta data block size mismatch"));
	m_ptr = std::malloc( blkbytesize);
	if (!m_ptr) throw std::bad_alloc();
	std::memcpy( m_ptr, blk_, blkbytesize);
}

MetaDataBlock::MetaDataBlock( const MetaDataBlock& o)
	:m_descr(o.m_descr),m_blockno(o.m_blockno),m_ptr(0)
{
	std::size_t blkbytesize = m_descr->bytesize() * BlockSize;
	m_ptr = std::malloc( blkbytesize);
	if (!m_ptr) throw std::bad_alloc();
	std::memcpy( m_ptr, o.m_ptr, blkbytesize);
}

void MetaDataBlock::swap( MetaDataBlock& o)
{
	std::swap( m_ptr, o.m_ptr);
	std::swap( m_descr, o.m_descr);
	std::swap( m_blockno, o.m_blockno);
}

MetaDataBlock::~MetaDataBlock()
{
	if (m_ptr) std::free( m_ptr);
}

void MetaDataBlock::init( const MetaDataDescription* descr_, 
				const Index& blockno_)
{
	m_blockno = blockno_;
	if (m_descr == descr_ && m_ptr)
	{
		std::memset( m_ptr, 0, m_descr->bytesize() * BlockSize);
	}
	else
	{
		m_descr = descr_;
		if (m_ptr) std::free( m_ptr);
		m_ptr = std::calloc( BlockSize, descr_->bytesize());
	}
}

void MetaDataBlock::init( const MetaDataDescription* descr_, 
				const Index& blockno_,
				const char* blk_,
				std::size_t blksize_)
{
	std::size_t blkbytesize = descr_->bytesize() * BlockSize;
	if (blksize_ != blkbytesize) throw std::runtime_error( _TXT( "meta data block size mismatch"));
	void* mem = std::malloc( BlockSize * descr_->bytesize());
	if (!mem) throw std::bad_alloc();
	m_descr = descr_;
	m_blockno = blockno_;
	if (m_ptr) std::free( m_ptr);
	m_ptr = mem;
	std::memcpy( m_ptr, blk_, blkbytesize);
}

const MetaDataRecord MetaDataBlock::operator[]( std::size_t idx) const
{
	if (idx >= BlockSize) throw strus::runtime_error( _TXT( "array bound read in meta data block"));
	void* recaddr = (char*)m_ptr + (idx * m_descr->bytesize());
	return MetaDataRecord( m_descr, recaddr);
}


