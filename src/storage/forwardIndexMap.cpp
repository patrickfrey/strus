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
#include "forwardIndexMap.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"

using namespace strus;

void ForwardIndexMap::closeCurblock( const Index& typeno, CurblockElemList& elemlist)
{
	if (elemlist.empty()) return;
	Index lastpos = elemlist.back().first;

	MapKey key( typeno, m_docno, lastpos);

	ForwardIndexBlock blk;
	blk.setId( lastpos);

	CurblockElemList::const_iterator ei = elemlist.begin(), ee = elemlist.end();
	for (; ei != ee; ++ei)
	{
		blk.append( ei->first, ei->second);
	}
	m_map[ key] = m_blocklist.size();
	m_blocklist.push_back( blk);
	elemlist.clear();
}

void ForwardIndexMap::closeCurblocks()
{
	CurblockMap::iterator bi = m_curblockmap.begin(), be = m_curblockmap.end();
	for (; bi != be; ++bi)
	{
		closeCurblock( bi->first, bi->second);
	}
}

void ForwardIndexMap::openForwardIndexDocument( const Index& docno)
{
	closeCurblocks();
	m_docno = docno;
}

void ForwardIndexMap::closeForwardIndexDocument()
{
	closeCurblocks();
	m_docno = 0;
}

void ForwardIndexMap::renameNewDocNumbers( const std::map<Index,Index>& renamemap)
{
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index docno = BlockKey( MapKey(mi->first).termkey).elem(2);
		if (KeyMap::isUnknown( docno))
		{
			Index typeno = BlockKey( MapKey(mi->first).termkey).elem(1);
			Index maxpos = MapKey(mi->first).maxpos;
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "forward index map");
			}
			MapKey newkey( typeno, ri->second, maxpos);
			m_map[ newkey] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void ForwardIndexMap::defineForwardIndexTerm(
	const Index& typeno,
	const Index& pos,
	const std::string& termstring)
{
	if (m_maxtype < typeno)
	{
		m_maxtype = typeno;
	}

	CurblockMap::iterator bi = m_curblockmap.find( typeno);
	if (bi == m_curblockmap.end())
	{
		m_curblockmap[ typeno] = CurblockElemList();
		bi = m_curblockmap.find( typeno);
		bi->second.reserve( m_maxblocksize);
	}
	else if (bi->second.size() >= m_maxblocksize)
	{
		closeCurblock( typeno, bi->second);
	}
	m_strings.push_back( termstring.c_str(), termstring.size());
	bi->second.push_back( CurblockElem( pos, m_strings.back()));
}

void ForwardIndexMap::deleteIndex( const Index& docno)
{
	if (docno == m_docno)
	{
		m_curblockmap.clear();
		m_docno = 0;
	}
	else
	{
		Index ti = 1, te = m_maxtype+1;
		for (; ti != te; ++ti)
		{
			BlockKeyIndex termkey = BlockKey( ti, docno).index();
			MapKey key( ti, docno, 0);
	
			Map::iterator fi = m_map.upper_bound( key);
			while (fi != m_map.end() && fi->first.termkey == termkey)
			{
				m_map.erase( fi++);
			}
		}
	}
	m_deletes.push_back( docno);
}

void ForwardIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	closeCurblocks();

	// [1] Write deletes:
	std::vector<Index>::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		Index ti = 1, te = m_maxtype+1;
		for (; ti != te; ++ti)
		{
			DatabaseAdapter_ForwardIndex::Writer dbadapter( m_database, ti, *di);
			dbadapter.removeSubTree( transaction);
		}
	}

	// [2] Write inserts:
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Map::const_iterator ei = mi, ee = mi;
		for (; ee != me && ee->first.termkey == mi->first.termkey; ++ee){}
		mi = ee;

		BlockKey key(ei->first.termkey);
		DatabaseAdapter_ForwardIndex::Writer dbadapter( m_database, key.elem(1), key.elem(2));

		// [2.1] Delete all old blocks:
		dbadapter.removeSubTree( transaction);

		// [2.2] Write the new blocks:
		for (; ei != ee; ++ei)
		{
			dbadapter.store( transaction, m_blocklist[ ei->second]);
		}
	}

	// [3] Clear maps:
	clear();
}

void ForwardIndexMap::clear()
{
	m_map.clear();
	m_blocklist.clear();
	m_curblockmap.clear();
	m_strings.clear();
	m_docno = 0;
	m_deletes.clear();
}


