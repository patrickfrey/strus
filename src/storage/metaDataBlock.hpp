/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_BLOCK_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_BLOCK_HPP_INCLUDED
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
	MetaDataBlock( const MetaDataDescription* descr_, const Index& blockno_);
	MetaDataBlock( const MetaDataDescription* descr_, 
			const Index& blockno_,
			const char* blk_,
			std::size_t blksize_);

	MetaDataBlock( const MetaDataBlock& o);
	~MetaDataBlock();

	void swap( MetaDataBlock& o);
	bool empty() const						{return m_ptr==0;}
	void init( const MetaDataDescription* descr_, 
			const Index& blockno_,
			const char* blk_,
			std::size_t blksize_);

	void init( const MetaDataDescription* descr_, const Index& blockno_);

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
		BlockShift=8		///< number of bits in BlockSize
	};

private:
	const MetaDataDescription* m_descr;
	Index m_blockno;
	void* m_ptr;
};

}
#endif

