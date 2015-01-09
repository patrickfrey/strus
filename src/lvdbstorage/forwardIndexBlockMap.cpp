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
#include "forwardIndexBlockMap.hpp"
#include "keyMap.hpp"

using namespace strus;

void ForwardIndexBlockMap::closeCurblock( const Index& typeno, CurblockElemList& elemlist)
{
	if (elemlist.empty()) return;
	Index lastpos = elemlist.back().first;

	MapKey key( typeno, m_docno, lastpos);

	ForwardIndexBlock& blk = m_map[ key];
	blk.setId( lastpos);

	CurblockElemList::const_iterator ei = elemlist.begin(), ee = elemlist.end();
	for (; ei != ee; ++ei)
	{
		blk.append( ei->first, ei->second);
	}
	elemlist.clear();
}

void ForwardIndexBlockMap::closeCurblocks()
{
	CurblockMap::iterator bi = m_curblockmap.begin(), be = m_curblockmap.end();
	for (; bi != be; ++bi)
	{
		closeCurblock( bi->first, bi->second);
	}
}

void ForwardIndexBlockMap::closeForwardIndexDocument( const Index& docno)
{
	closeCurblocks();
	if (m_docno != docno)
	{
		throw std::runtime_error( "forward document operations not grouped by document");
	}
}

void ForwardIndexBlockMap::defineForwardIndexTerm(
	const Index& typeno,
	const Index& docno,
	const Index& pos,
	const std::string& termstring)
{
	if (m_maxtype < typeno)
	{
		m_maxtype = typeno;
	}
	if (m_docno != 0 && m_docno != docno)
	{
		closeCurblocks();
	}
	m_docno = docno;

	CurblockMap::iterator bi = m_curblockmap.find( typeno);
	if (bi == m_curblockmap.end())
	{
		m_curblockmap[ typeno] = CurblockElemList();
		bi = m_curblockmap.find( typeno);
	}

	if (bi->second.size() > ForwardIndexBlock::MaxBlockTokens)
	{
		closeCurblock( typeno, bi->second);
	}
	bi->second.push_back( CurblockElem( pos, termstring));
}

void ForwardIndexBlockMap::deleteDocument( const Index& docno)
{
	if (docno == m_docno)
	{
		m_curblockmap.clear();
		m_docno = 0;
	}
	Index ti = 0, te = m_maxtype;
	for (; ti != te; ++ti)
	{
		BlockKeyIndex termkey = BlockKey( ti, m_docno).index();
		MapKey key( ti, m_docno, 0);

		Map::iterator fi = m_map.upper_bound( key);
		while (fi != m_map.end() && fi->first.termkey == termkey)
		{
			m_map.erase( fi++);
		}
	}
	m_deletes.push_back( docno);
}

void ForwardIndexBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	closeCurblocks();

	// [1] Get deletes:
	KeyValueStorage fwstorage( m_db, DatabaseKey::ForwardIndexPrefix, false);
	std::vector<Index>::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		Index ti = 0, te = m_maxtype;
		for (; ti != te; ++ti)
		{
			fwstorage.disposeSubnodes( BlockKey( ti, *di), batch);
		}
	}

	// [2] Get inserts:
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Map::const_iterator ei = mi, ee = mi;
		for (; ee != me && ee->first.termkey == mi->first.termkey; ++ee){}
		mi = ee;

		BlockStorage<ForwardIndexBlock> blkstorage(
				m_db, DatabaseKey::ForwardIndexPrefix,
				BlockKey(ei->first.termkey), false);
		const ForwardIndexBlock* blk;

		// [1] Delete all old blocks with the database key as prefix address:
		for (blk = blkstorage.load( 0);
			blk != 0; blk = blkstorage.loadNext())
		{
			blkstorage.dispose( blk->id(), batch);
		}

		// [2] Write the new blocks:
		for (; ei != ee; ++ei)
		{
			blkstorage.store( ei->second, batch);
		}
	}
	// [3] clear maps:
	m_map.clear();
	m_curblockmap.clear();
	m_docno = 0;
	m_deletes.clear();
}

