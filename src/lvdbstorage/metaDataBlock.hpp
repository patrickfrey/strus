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
#ifndef _STRUS_LVDB_METADATA_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataRecord.hpp"
#include "metaDataDescription.hpp"
#include <utility>
#include <cstdlib>

namespace strus {

class MetaDataBlock
{
public:
	MetaDataBlock();
	MetaDataBlock( const MetaDataDescription* descr_, unsigned int blockno_);
	MetaDataBlock( const MetaDataDescription* descr_, 
			unsigned int blockno_,
			const char* blk_,
			std::size_t blksize_);

	MetaDataBlock( const MetaDataBlock& o);

	~MetaDataBlock();

	void init( const MetaDataDescription* descr_, 
			unsigned int blockno_,
			const char* blk_,
			std::size_t blksize_);

	static std::size_t index( const Index& docno)			{return (docno-1) & BlockMask;}
	static Index blockno( const Index& docno)			{return ((docno-1)>>BlockShift)+1;}

	Index blockno() const						{return m_blockno;}

	const MetaDataRecord operator[]( std::size_t idx) const;

	const void* ptr() const						{return m_ptr;}
	const char* charptr() const					{return (const char*)m_ptr;}
	std::size_t bytesize() const					{return m_descr->bytesize() * BlockSize;}

public:
	enum {
		/// \remark This value limits the maximum docno possible to (MetaDataBlockCache::CacheSize * MetaDataBlockSize)
		BlockSize=256		///< number of records in one meta data block
	};
private:
	enum {
		BlockMask=((int)BlockSize-1),
		BlockShift=8
	};

private:
	const MetaDataDescription* m_descr;
	unsigned int m_blockno;
	void* m_ptr;
};

}
#endif

