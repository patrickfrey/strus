/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "forwardIndexMap.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"

using namespace strus;

void ForwardIndexMap::closeCurblock( const Index& typeno, const CurblockElemList& elemlist)
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
}

void ForwardIndexMap::closeCurblocks()
{
	CurblockMap::iterator bi = m_curblockmap.begin(), be = m_curblockmap.end();
	for (; bi != be; ++bi)
	{
		closeCurblock( bi->first, bi->second);
		bi->second.clear();
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
	{
		// Rename document numbers in forward index block map:
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
	}{
		// Rename document numbers in list of forward index block deletes:
		std::set<Index>::iterator
			di = m_docno_deletes.upper_bound( KeyMap::unknownUpperBoundKey()),
			de = m_docno_deletes.end();
		while (di != de)
		{
			Index docno = *di;
			if (KeyMap::isUnknown( docno))
			{
				std::set<Index>::iterator next_di = di;
				++next_di;

				std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
				if (ri != renamemap.end())
				{
					m_docno_deletes.insert( ri->second);
				}
				m_docno_deletes.erase( di);
				di = next_di;
			}
			else
			{
				++di;
			}
		}
	}{
		// Rename document numbers in list of partial forward index block deletes:
		std::map<Index, std::set<Index> >::iterator
			di = m_docno_typeno_deletes.upper_bound( KeyMap::unknownUpperBoundKey()),
			de = m_docno_typeno_deletes.end();
		for (; di != de; ++di)
		{
			Index docno = di->first;
			if (KeyMap::isUnknown( docno))
			{
				std::map<Index, std::set<Index> >::iterator next_di = di;
				++next_di;

				std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
				if (ri != renamemap.end())
				{
					m_docno_typeno_deletes.insert( std::pair<Index, std::set<Index> >(ri->second, di->second));
				}
				m_docno_typeno_deletes.erase( di);
				di = next_di;
			}
			else
			{
				++di;
			}
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
		bi->second.clear();
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
	if (m_docno_typeno_deletes.find( docno) != m_docno_typeno_deletes.end())
	{
		m_docno_typeno_deletes.erase( docno);
	}
	m_docno_deletes.insert( docno);
}

void ForwardIndexMap::deleteIndex( const Index& docno, const Index& typeno)
{
	if (docno == m_docno)
	{
		m_curblockmap[ typeno] = CurblockElemList();
		m_docno = 0;
	}
	else
	{
		BlockKeyIndex termkey = BlockKey( typeno, docno).index();
		MapKey key( typeno, docno, 0);
	
		Map::iterator fi = m_map.upper_bound( key);
		while (fi != m_map.end() && fi->first.termkey == termkey)
		{
			m_map.erase( fi++);
		}
	}
	if (m_docno_deletes.find( docno) == m_docno_deletes.end())
	{
		m_docno_typeno_deletes[ docno].insert( typeno);
	}
}

void ForwardIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	closeCurblocks();

	// [1] Write deletes:
	std::set<Index>::const_iterator di = m_docno_deletes.begin(), de = m_docno_deletes.end();
	for (; di != de; ++di)
	{
		Index ti = 1, te = m_maxtype+1;
		for (; ti != te; ++ti)
		{
			DatabaseAdapter_ForwardIndex::Writer dbadapter( m_database, ti, *di);
			dbadapter.removeSubTree( transaction);
		}
	}
	std::map<Index, std::set<Index> >::const_iterator ui = m_docno_typeno_deletes.begin(), ue = m_docno_typeno_deletes.end();
	for (; ui != ue; ++ui)
	{
		std::set<Index>::const_iterator ti = ui->second.begin(), te = ui->second.end();
		for (; ti != te; ++ti)
		{
			DatabaseAdapter_ForwardIndex::Writer dbadapter( m_database, *ti, ui->first);
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
}

void ForwardIndexMap::clear()
{
	m_map.clear();
	m_blocklist.clear();
	m_curblockmap.clear();
	m_strings.clear();
	m_docno = 0;
	m_docno_deletes.clear();
	m_docno_typeno_deletes.clear();
}

