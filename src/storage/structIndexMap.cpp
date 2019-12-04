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
#include "structBlockBatchWrite.hpp"
#include <algorithm>
#include <limits>

using namespace strus;

StructIndexMap::StructIndexMap( DatabaseClientInterface* database_, const Index& maxstructo_)
	:m_database(database_),m_defar(),m_mapar(maxstructo_),m_docno(0)
{}

#define DELETE_STRUCTURES -1

void StructIndexMap::clear()
{
	m_docno = 0;
	m_defar.clear();
	m_mapar.clear();
}

void StructIndexMap::reset()
{
	std::size_t maparsize = m_mapar.size();
	clear();
	m_mapar.resize( maparsize);
}

void StructIndexMap::defineStructure(
	const Index& structno,
	const Index& docno,
	const IndexRange& source,
	const IndexRange& sink)
{
	if (structno <= 0)
	{
		throw strus::runtime_error(_TXT("internal: '%s' not defined"), "structno");
	}
	if (structno > (Index)m_mapar.size()) m_mapar.resize( structno);

	if (docno <= 0) throw strus::runtime_error(_TXT("internal: invalid '%s'"), "docno");
	if (!source.defined()) throw strus::runtime_error(_TXT("try to add structure with empty %s"), "source");
	if (!sink.defined()) throw strus::runtime_error(_TXT("try to add structure with empty %s"), "sink");
	if (source.end() > std::numeric_limits<StructBlock::PositionType>::max()) throw strus::runtime_error(_TXT("structure %s exceeds maximum position stored"), "source");
	if (sink.end() > std::numeric_limits<StructBlock::PositionType>::max()) throw strus::runtime_error(_TXT("structure %s exceeds maximum position stored"), "sink");

	StructDefSet* defset = 0;
	Map::iterator mi = m_mapar[ structno-1].find( docno);

	if (mi == m_mapar[ structno-1].end() || mi->second == DELETE_STRUCTURES)
	{
		m_mapar[ structno-1][ docno] = m_defar.size();
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
	defset->insert( StructDef( source, sink));
	m_docno = docno;
}

void StructIndexMap::deleteInsertedStructs( const Index& docno, const Index& structno)
{
	if (structno > (Index)m_mapar.size() || structno <= 0)
	{
		throw strus::runtime_error(_TXT("internal: '%s' not defined"), "structno");
	}
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

void StructIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	Index si = 0, se = m_mapar.size();
	for (; si != se; ++si)
	{
		Index structno = si+1;
		DatabaseAdapter_StructBlock::WriteCursor dbadapter( m_database, structno);
		std::vector<StructBlockBuilder::StructDeclaration> declar;

		const Map& map = m_mapar[ si];
		Map::const_iterator mi = map.begin(), me = map.end();
		for (; mi != me; ++mi)
		{
			strus::Index docno = mi->first;
			if (mi->second == DELETE_STRUCTURES)
			{
				declar.push_back( StructBlockBuilder::StructDeclaration(
					docno, IndexRange(), IndexRange()));
			}
			else
			{
				if (mi->second >= (int)m_defar.size()) throw strus::runtime_error(_TXT("corrupt data, array bound read in %s"), "structure index map");

				StructDefSet defset = m_defar[ mi->second];
				StructDefSet::const_iterator di = defset.begin(), de = defset.end();
				for (; di != de; ++di)
				{
					declar.push_back( StructBlockBuilder::StructDeclaration(
						docno, di->source, di->sink));
				}
			}
		}
		StructBlockBuilder newblk;
		std::vector<StructBlockBuilder::StructDeclaration>::const_iterator
			ei = declar.begin(), ee = declar.end();

		// [1] Merge new elements with existing upper bound blocks:
		StructBlockBatchWrite::mergeNewElements( &dbadapter, ei, ee, newblk, transaction);

		// [2] Write the new blocks that could not be merged into existing ones:
		StructBlockBatchWrite::insertNewElements( &dbadapter, ei, ee, newblk, transaction);
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
				out << " []" << std::endl;
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


