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
#include "structBlock.hpp"
#include "keyMap.hpp"
#include <algorithm>
#include <limits>

using namespace strus;

StructIndexMap::StructIndexMap( DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_database(database_),m_builderar(),m_docnomap()
{}

#define DELETE_STRUCTURES -1

void StructIndexMap::clear()
{
	m_builderar.clear();
	m_docnomap.clear();
}

void StructIndexMap::defineStructure(
	strus::Index structno,
	strus::Index docno,
	const IndexRange& source,
	const IndexRange& sink)
{
	if (structno <= 0) throw strus::runtime_error(_TXT("internal: '%s' not defined"), "structno");
	if (docno <= 0) throw strus::runtime_error(_TXT("internal: invalid '%s'"), "docno");
	if (!source.defined()) throw strus::runtime_error(_TXT("try to add structure with empty %s"), "source");
	if (!sink.defined()) throw strus::runtime_error(_TXT("try to add structure with empty %s"), "sink");
	if (source.end() > std::numeric_limits<StructBlock::PositionType>::max()) throw strus::runtime_error(_TXT("structure %s exceeds maximum position stored"), "source");
	if (sink.end() > std::numeric_limits<StructBlock::PositionType>::max()) throw strus::runtime_error(_TXT("structure %s exceeds maximum position stored"), "sink");

	StructBlockBuilder* builder = 0;
	DocnoMap::iterator mi = m_docnomap.find( docno);

	if (mi == m_docnomap.end() || mi->second == DELETE_STRUCTURES)
	{
		m_docnomap[ docno] = m_builderar.size();
		m_builderar.push_back( StructBlockBuilder( docno));
		builder = &m_builderar.back();
	}
	else
	{
		builder = &m_builderar[ mi->second];
	}
	builder->append( structno, source, sink);
}

void StructIndexMap::deleteIndex( strus::Index docno)
{
	DocnoMap::iterator mi = m_docnomap.find( docno);
	if (mi != m_docnomap.end())
	{
		if (mi->second != DELETE_STRUCTURES)
		{
			m_builderar[ mi->second].clear();
			m_docnomap[ docno] = DELETE_STRUCTURES;
		}
	}
	else
	{
		m_docnomap[ docno] = DELETE_STRUCTURES;
	}
}

void StructIndexMap::renameNewDocNumbers( const std::map<strus::Index,strus::Index>& docnoUnknownMap)
{
	DocnoMap docnomap_new;
	DocnoMap::iterator ai = m_docnomap.begin(), ae = m_docnomap.end();
	for (; ai != ae; ++ai)
	{
		Index new_docno;
		if (KeyMap::isUnknown( ai->first/*docno*/))
		{
			std::map<strus::Index,strus::Index>::const_iterator
				ri = docnoUnknownMap.find( ai->first/*docno*/);
			if (ri == docnoUnknownMap.end())
			{
				throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "struct map");
			}
			new_docno = ri->second;
		}
		else
		{
			new_docno = ai->first/*docno*/;
		}
		docnomap_new[ new_docno] = ai->second;
	}
	std::vector<StructBlockBuilder>::iterator
		bi = m_builderar.begin(), be = m_builderar.end();
	for (; bi != be; ++bi)
	{
		if (KeyMap::isUnknown( bi->docno()))
		{
			std::map<strus::Index,strus::Index>::const_iterator
				ri = docnoUnknownMap.find( bi->docno());
			if (ri == docnoUnknownMap.end())
			{
				throw strus::runtime_error( _TXT( "%s value undefined (%s)"), "docno", "struct map");
			}
			bi->setDocno( ri->second);
		}
	}
	m_docnomap.swap( docnomap_new);
}

void StructIndexMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	DocnoMap::const_iterator ai = m_docnomap.begin(), ae = m_docnomap.end();
	for (; ai != ae; ++ai)
	{
		DatabaseAdapter_StructBlock::WriteCursor dbadapter( m_database);
		if (ai->second == DELETE_STRUCTURES)
		{
			dbadapter.remove( transaction, ai->first);
		}
		else
		{
			StructBlock blk( m_builderar[ ai->second].createBlock());
			dbadapter.store( transaction, blk);
		}
	}
}

void StructIndexMap::print( std::ostream& out) const
{
	out << "[docno] to structure members map:" << std::endl;
	DocnoMap::const_iterator ai = m_docnomap.begin(), ae = m_docnomap.end();
	for (; ai != ae; ++ai)
	{
		if (ai->second == DELETE_STRUCTURES)
		{
			out << "DELETE docno " << ai->first << std::endl;
		}
		else
		{
			out << "STORE ";
			m_builderar[ ai->second].print( out);
			out << std::endl;
		}
	}
}


