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
#ifndef _STRUS_LVDB_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "forwardIndexBlock.hpp"
#include "blockKey.hpp"
#include "localStructAllocator.hpp"
#include <vector>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class ForwardIndexBlockMap
{
public:
	explicit ForwardIndexBlockMap( DatabaseInterface* database_)
		:m_database(database_),m_docno(0),m_maxtype(0){}

	void defineForwardIndexTerm(
		const Index& typeno,
		const Index& docno,
		const Index& pos,
		const std::string& termstring);

	void closeForwardIndexDocument( const Index& docno);

	void deleteIndex( const Index& docno);
	void getWriteBatch( DatabaseTransactionInterface* transaction);

private:
	struct MapKey
	{
		BlockKeyIndex termkey;
		Index maxpos;

		MapKey( const MapKey& o)
			:termkey(o.termkey),maxpos(o.maxpos){}
		MapKey( const Index& typeno_, const Index& docno_, const std::size_t& maxpos_)
			:termkey(BlockKey(typeno_,docno_).index()),maxpos(maxpos_){}

		bool operator < (const MapKey& o) const
		{
			if (termkey < o.termkey) return true;
			if (termkey > o.termkey) return false;
			return maxpos < o.maxpos;
		}
	};

	typedef LocalStructAllocator<std::pair<MapKey,ForwardIndexBlock> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,ForwardIndexBlock,MapCompare,MapAllocator> Map;

	typedef std::pair<Index,std::string> CurblockElem;
	typedef std::vector<CurblockElem> CurblockElemList;
	typedef LocalStructAllocator<std::pair<Index,CurblockElemList> > CurblockMapAllocator;
	typedef std::map<Index,CurblockElemList,std::less<Index>,CurblockMapAllocator> CurblockMap;

private:
	void closeCurblock( const Index& typeno, CurblockElemList& blk);
	void closeCurblocks();

private:
	DatabaseInterface* m_database;
	Map m_map;
	CurblockMap m_curblockmap;
	Index m_docno;
	Index m_maxtype;
	std::vector<Index> m_deletes;
};

}
#endif

