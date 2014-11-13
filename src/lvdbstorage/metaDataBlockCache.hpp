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
#ifndef _STRUS_LVDB_METADATA_BLOCK_CACHE_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_BLOCK_CACHE_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataBlock.hpp"
#include <utility>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace strus {

class MetaDataBlockCache
{
public:
	MetaDataBlockCache( leveldb::DB* db_);

	~MetaDataBlockCache(){}

	void resetBlock( Index blockno_, char varname_);

	float getValue( Index docno_, char varname_);

private:
	enum {
		VarnameMin=33,
		VarnameMax=127,
		VarnameDim=(VarnameMax-VarnameMin+1)
	};

	std::size_t aridx( char varname)
	{
		unsigned char rt = (unsigned char)varname-VarnameMin;
		if (rt > (unsigned char)VarnameDim)
		{
			throw std::runtime_error("internal: metadata variable out of range");
		}
		return (std::size_t)rt;
	}

private:
	struct BlockRef
	{
		boost::shared_ptr<MetaDataBlock> ref;
		boost::mutex mutex;

		BlockRef(){}
	};

	struct BlockArray
	{
		boost::shared_array<BlockRef> ar;
		std::size_t arsize;

		BlockArray()
			:arsize(0){}

		void resize( std::size_t nofBlocks_);

		MetaDataBlockRef& operator[]( std::size_t idx);
		MetaDataBlockRef operator[]( std::size_t idx) const;
	};

private:
	leveldb::DB* m_db;
	BockArray m_ar[VarnameMax-VarnameMin+1];
};

}
#endif

