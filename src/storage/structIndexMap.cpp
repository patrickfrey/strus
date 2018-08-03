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
#include <algorithm>

using namespace strus;

StructIndexMap::StructIndexMap( DatabaseClientInterface* database_)
	:m_database(database_),m_map(),m_docno(0),m_structnoset(),m_docno_deletes(),m_structno_docno_deletes()
{
}

void StructIndexMap::clear()
{
	m_map.clear();
	m_docno = 0;
	m_structnoset.clear();
	m_docno_deletes.clear();
	m_structno_docno_deletes.clear();
}

void StructIndexMap::defineStructure(
	const Index& structno,
	const Index& docno,
	const IndexRange& source,
	const IndexRange& sink)
{
	MapKey key( structno, docno, source.end(), sink.end());
	MapValue val( source.start(), sink.start());
	m_structnoset.insert( structno);
	m_map.insert( Map::value_type( key, val));
}

void StructIndexMap::deleteIndex( const Index& docno)
{
	std::set<Index>::const_iterator si = m_structnoset.begin(), se = m_structnoset.end();
	for (; si != se; ++si)
	{
		MapKey key( *si, docno, 0, 0);
		Map::const_iterator mi = m_map.upper_bound( key);
		Map::const_iterator me = mi;
		for (; me != m_map.end() && me->docno == docno; ++me)
		m_map.erase( mi, me);
		m_structno_docno_deletes.insert( DocStructRel( *si, docno));
	}
}

void StructIndexMap::deleteIndex( const Index& docno, const Index& structno)
{
	MapKey key( structno, docno, 0, 0);
	Map::const_iterator mi = m_map.upper_bound( key);
	Map::const_iterator me = mi;
	for (; me != m_map.end() && me->docno == docno; ++me)
	m_map.erase( mi, me);
	m_docno_deletes.insert( docno);
}

void StructIndexMap::renameNewNumbers( const std::map<Index,Index>& docnoUnknownMap)
{
	{
		Map newmap;
		Map::iterator mi = m_map.begin(), me = m_map.end();
		for (; mi != me; ++mi)
		{
			Index docno = mi->first.docno;
			if (KeyMap::isUnknown( docno))
			{
				Index new_docno;
				std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( docno);
				if (ri == docnoUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "posinfo map");
				}
				new_docno = ri->second;
			}
			else
			{
				new_docno = docno;
			}
			MapKey newkey( mi->structno, new_docno, mi->source_end, mi->sink_end);
			newmap[ newkey] = mi->second;
		}
		m_map = newmap;
	}{
		std::vector<Index> eraselist;
		std::set<Index>::const_iterator di = m_docno_deletes.begin(), de = m_docno_deletes.end();
		for (; di != de; ++di)
		{
			Index docno = *di;
			if (KeyMap::isUnknown( docno))
			{
				eraselist.push_back( docno);
				std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( docno);
				if (ri == docnoUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "posinfo map");
				}
				m_docno_deletes.insert( ri->second);
				eraselist.push_back( docno);
			}
		}
		std::vector<Index>::const_iterator ei = eraselist.begin(), ee = eraselist.end();
		for (; ei != ee; ++ei)
		{
			m_docno_deletes.erase( *ei);
		}
	}{
		std::set<StructDocRel> new_structno_docno_deletes;
		std::set<StructDocRel>::const_iterator di = m_structno_docno_deletes.begin(), de = m_structno_docno_deletes.end();
		for (; di != de; ++di)
		{
			if (KeyMap::isUnknown( di->docno))
			{
				std::map<Index,Index>::const_iterator ri = docnoUnknownMap.find( docno);
				if (ri == docnoUnknownMap.end())
				{
					throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "posinfo map");
				}
				new_structno_docno_deletes.insert( StructDocRel( ri->second/*docno*/, di->structno));
			}
			else
			{
				new_structno_docno_deletes.insert( *di);
			}
		}
		m_structno_docno_deletes = new_structno_docno_deletes;
	}
}

void StructIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
}

void StructIndexMap::print( std::ostream& out) const
{
}

void StructIndexMap::insertNewElements(
		DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		StructBlockBuilder& newblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
}

void StructIndexMap::mergeNewElements(
		DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		StructBlockBuilder& newblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
}

void StructIndexMap::mergeBlock(
		DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct, 
		DatabaseTransactionInterface* transaction,
		Map::const_iterator ei,
		const Map::const_iterator& ee,
		const StructBlock& oldblk,
		StructBlockBuilder& newblk)
{
}

