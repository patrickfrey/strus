/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structIndexMap.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include "keyMap.hpp"
#include <algorithm>

using namespace strus;

StructIndexMap::StructIndexMap( DatabaseClientInterface* database_, Index maxstructo_)
	:m_database(database_),m_defar(),m_mapar(maxstructo_),m_docno(0)
{}

#define DELETE_STRUCTURES (Index)-1

void StructIndexMap::clear()
{
	m_docno = 0;
	m_defar.clear();
	m_mapar.clear();
}

void StructIndexMap::defineStructure(
	const Index& structno,
	const Index& docno,
	const IndexRange& source,
	const IndexRange& sink)
{
	if (structno > (Index)m_mapar.size() || structno <= 0) throw strus::runtime_error(_TXT("internal: '%s' not defined"), "structno");
	if (docno <= 0) throw strus::runtime_error(_TXT("internal: invalid '%s'"), "docno");
	StructDefSet* defset = 0;

	Map& map = m_mapar[ structno-1];
	Map::iterator mi = map.find( docno);

	if (mi == map.end() || mi->second == DELETE_STRUCTURES)
	{
		map[ docno] = m_defar.size();
		m_defar.push_back( StructDefSet());
		defset = &m_defar.back();
	}
	else
	{
		if (!m_docno || m_docno != docno)
		{
			m_defar[ mi->second].clear();
		}
		defset = &m_defar[ mi->second];
	}
	m_docno = docno;

	defset->insert( StructDef( source, sink));
}

void StructIndexMap::deleteInsertedStructs( const Index& docno, const Index& structno)
{
	if (structno > (Index)m_mapar.size() || structno <= 0) throw strus::runtime_error(_TXT("internal: '%s' not defined"), "structno");
	if (docno <= 0) throw strus::runtime_error(_TXT("internal: invalid '%s'"), "docno");
	m_mapar[ structno-1][ docno] = DELETE_STRUCTURES;
}

void StructIndexMap::deleteIndex( const Index& docno)
{
	for (int si=0; si < (int)m_mapar.size(); ++si)
	{
		Index structno = si+1;
		deleteInsertedStructs( docno, structno);
	}
}

void StructIndexMap::deleteIndex( const Index& docno, const Index& structno)
{
	deleteInsertedStructs( docno, structno);
}

void StructIndexMap::renameNewNumbers( const std::map<Index,Index>& docnoUnknownMap)
{
	std::vector<Map>::iterator ai = m_mapar.begin(), ae = m_mapar.end();
	for (; ai != ae; ++ai)
	{
		Map newmap;
		Map::iterator mi = ai->begin(), me = ai->end();
		for (; mi != me; ++mi)
		{
			Index new_docno;
			if (KeyMap::isUnknown( mi->first/*docno*/))
			{
				std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( mi->first/*docno*/);
				if (ri == docnoUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "posinfo map");
				}
				new_docno = ri->second;
			}
			else
			{
				new_docno = mi->first/*docno*/;
			}
			newmap[ new_docno] = mi->second;
		}
		*ai = newmap;
	}
}

bool StructIndexMap::fitsNofStructuresLeft( Map::const_iterator mi, const Map::const_iterator& me, int maxLimit) const
{
	int nn = 0;
	while (mi != me && nn < maxLimit)
	{
		int idx = mi->second;
		if (idx == DELETE_STRUCTURES) continue;
		nn += m_defar[ idx].size();
	}
	return nn < maxLimit;
}

void StructIndexMap::writeNewBlocks(
		DatabaseAdapter_StructBlock::WriteCursor& dbadapter,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator mi,
		const Map::const_iterator& me,
		StructBlockBuilder& blk)
{
	Index docno = 0;
	while (mi != me)
	{
		docno = mi->second;
		int idx = mi->second;
		if (idx == DELETE_STRUCTURES) continue;
		const StructDefSet& st = m_defar[ idx];
		int nofDocStructs = st.size();
		if (blk.empty() || blk.fitsInto( nofDocStructs))
		{
			StructDefSet::const_iterator si = st.begin(), se = st.end();
			for (; si != se; ++si)
			{
				blk.push( docno, si->source, si->sink);
			}
		}
		else
		{
			blk.setId( docno);
			dbadapter.store( transaction, blk.createBlock());
			blk.clear();
		}
	}
}

void StructIndexMap::loadStoredElementsFromBlock( const Index& structno, const StructBlock& blk)
{
	DocIndexNodeCursor cursor;
	Index docno = blk.firstDoc( cursor);
	for (; docno; docno = blk.nextDoc( cursor))
	{
		Map& map = m_mapar[ structno-1];
		StructDefSet* defset = 0;
		Map::iterator mi = map.find( docno);
		if (mi == map.end())
		{
			map[ docno] = m_defar.size();
			m_defar.push_back( StructDefSet());
			defset = &m_defar.back();
		}
		else if (mi->second == DELETE_STRUCTURES)
		{
			continue;
		}
		else
		{
			defset = &m_defar[ mi->second];
		}
		if (defset->empty())
		{
			StructBlock::StructureScanner si = blk.structureScanner_at( cursor);
			const StructBlock::StructureDef* sdef = si.current();
			for (; sdef; si.next(),sdef = si.current())
			{
				IndexRange source( sdef->header_start, sdef->header_end);
				StructBlock::MemberScanner mitr = si.members();
				const StructBlock::StructureMember* member = mitr.current();
				for (;member; mitr.next(),member=mitr.current())
				{
					IndexRange sink( member->start, member->end);
					defset->insert( StructDef( source, sink));
				}
			}
		}
	}
}

void StructIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	Index si = 0, se = m_mapar.size();
	for (; si != se; ++si)
	{
		Index structno = si+1;
		DatabaseAdapter_StructBlock::WriteCursor dbadapter( m_database, structno);

		const Map& map = m_mapar[ si];
		Map::const_iterator mi = map.begin(), me = map.end();
		while (mi != me)
		{
			StructBlock blk;
			Index docno = mi->first;
			if (dbadapter.loadUpperBound( docno, blk))
			{
				DocIndexNodeCursor cursor;
				Index blk_first_docno = blk.firstDoc( cursor);
				Map::const_iterator next_mi = map.upper_bound( blk_first_docno-1);
				StructBlockBuilder restblk;
				writeNewBlocks( dbadapter, transaction, mi, me, restblk);
				if (restblk.size() + blk.size() <= StructBlock::MaxBlockSize)
				{
					loadStoredElementsFromBlock( structno, blk);
					dbadapter.remove( transaction, blk.id());
					mi = map.find( docno);
					me = map.end();
				}
				else
				{
					if (!restblk.empty()) dbadapter.store( transaction, restblk.createBlock());
					mi = next_mi;
				}
			}
			else
			{
				StructBlockBuilder restblk;
				writeNewBlocks( dbadapter, transaction, mi, me, restblk);
				if (!restblk.empty()) dbadapter.store( transaction, restblk.createBlock());
				mi = me;
			}
		}
	}
}

void StructIndexMap::print( std::ostream& out) const
{
	out << "[structno,docno] to structure members map:" << std::endl;
	Index si = 0, se = m_mapar.size();
	for (; si != se; ++si)
	{
		Index structno = si+1;
		const Map& map = m_mapar[ si];
		Map::const_iterator mi = map.begin(), me = map.end();
		for (; mi != me; ++mi)
		{
			Index docno = mi->first;
			out << strus::string_format("[structno=%d, docno=%d]", structno, docno);
			if (mi->second == DELETE_STRUCTURES)
			{
				out << " DELETE" << std::endl;
			}
			else
			{
				const StructDefSet& defset = m_defar[ mi->second];
				StructDefSet::const_iterator di = defset.begin(), de = defset.end();
				for (int didx=0; di != de; ++di,++didx)
				{
					if (didx) out << ", "; else out << " ";
					out << strus::string_format( "[%d,%d]->[%d,%d]", di->source.start(), di->source.end(), di->sink.start(), di->sink.end());
				}
				out << std::endl;
			}
		}
	}
}


