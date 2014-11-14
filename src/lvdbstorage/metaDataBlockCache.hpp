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
#include <cstdlib>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>

namespace strus {

class MetaDataBlockCache
{
public:
	MetaDataBlockCache( leveldb::DB* db_);

	~MetaDataBlockCache(){}

	float getValue( Index docno, char varname);

	void declareVoid( unsigned int blockno, char varname);
	void refresh();

private:
	struct VoidRef
	{
		unsigned int blockno;
		char varname;

		VoidRef() :blockno(0),varname(0){}
		VoidRef( const VoidRef& o) :blockno(o.blockno),varname(o.varname){}
		VoidRef( unsigned int blockno_, char varname_) :blockno(blockno_),varname(varname_){}
	};

	void resetBlock( unsigned int blockno, char varname);

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
	enum {
		NodeSize=1024,							///< size of one node in the cache
		MaxBlockno=(NodeSize*NodeSize),					///< hardcode limit of maximum document block number
		MaxDocno=(NodeSize*NodeSize*MetaDataBlock::MetaDataBlockSize)	///< hardcode limit of maximum document number
	};

	struct BlockRef
	{
		boost::mutex mutex;
		boost::shared_ptr<MetaDataBlock> ref;

		BlockRef(){}
	};

	template <class NodeType>
	struct NodeArray
	{
		enum {NofNodesPerMutex=16};
		boost::shared_ptr<NodeType> ar[ NodeSize];
		boost::mutex mutex[ NodeSize / NofNodesPerMutex];

		NodeArray(){}

		boost::shared_ptr<NodeType> operator[]( std::size_t index) const
		{
			if (index > NodeSize) throw std::logic_error("array bound read (MetaDataBlockCache::NodeArray)");
			return ar[ index];
		}

		struct Reference
		{
			Reference( boost::mutex* mutex, boost::shared_ptr<NodeType>* ref)
			{
				mutex->lock();
			}
			~Reference()
			{
				mutex->unlock();
			}

			boost::shared_ptr<NodeType>& content()
			{
				return *ref;
			}

		private:
			boost::mutex* mutex;
			boost::shared_ptr<NodeType>* ref;
		};

		Reference access( std::size_t index)
		{
			if (index > NodeSize) throw std::logic_error("array bound read (MetaDataBlockCache::NodeArray)");
			return Reference( &mutex[ index / NofNodesPerMutex], ar + index);
		}
	};
	typedef NodeArray<MetaDataBlock> BlockArray;
	typedef NodeArray<BlockArray> BlockNodeArray;

private:
	leveldb::DB* m_db;
	boost::shared_ptr<BlockNodeArray> m_ar[VarnameDim];
	std::vector<VoidRef> m_voidar;
};

}
#endif

