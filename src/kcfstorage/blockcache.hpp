/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_KCF_BLOCK_CACHE_HPP_INCLUDED
#define _STRUS_KCF_BLOCK_CACHE_HPP_INCLUDED
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>

namespace strus
{

///\class BlockChache
///\brief Implementation of a block cache with some sort of local LRU
class BlockCache
{
public:
	BlockCache( std::size_t nofblocks_, std::size_t blocksize_);
	virtual ~BlockCache();

	void* allocBlock( unsigned int id);
	void insertBlock( void* data);
	void* getBlock( unsigned int id);

	unsigned int blockId( void* data) const;

private:
	struct BlockMetaData
	{
		unsigned int id;
		boost::atomic<unsigned int> refcnt;
	};

	BlockMetaData* blockMetaData( void* block) const;
	void freeBlock( void* block) const;

	struct BlockRef
	{
		boost::atomic<void*> data;
		unsigned int timestamp;
	};
	unsigned int findLRU() const;

private:
	BlockRef* m_ar;
	std::size_t m_arsize;
	unsigned int* m_hashtable;
	unsigned int m_hashtablesize;
	unsigned int m_timestamp;
	std::size_t m_blocksize;
};

}//namespace
#endif

