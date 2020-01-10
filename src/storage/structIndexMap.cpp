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
	:m_errorhnd(errorhnd_),m_database(database_),m_blockar(),m_docnomap()
{}

#define DELETE_STRUCTURES -1

void StructIndexMap::clear()
{
	m_blockar.clear();
	m_docnomap.clear();
}

void StructIndexMap::defineStructureBlock( strus::Index docno, const StructBlock& blk)
{
	DocnoMap::iterator mi = m_docnomap.find( docno);
	if (mi == m_docnomap.end() || mi->second == DELETE_STRUCTURES)
	{
		m_docnomap[ docno] = m_blockar.size();
		m_blockar.push_back( blk);
	}
	else
	{
		m_blockar[ mi->second] = blk;
	}
}

void StructIndexMap::deleteIndex( strus::Index docno)
{
	DocnoMap::iterator mi = m_docnomap.find( docno);
	if (mi != m_docnomap.end())
	{
		if (mi->second != DELETE_STRUCTURES)
		{
			m_blockar[ mi->second].clear();
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
			m_blockar[ ai->second].setId( ai->first);
			dbadapter.store( transaction, m_blockar[ ai->second]);
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
			m_blockar[ ai->second].print( out);
			out << std::endl;
		}
	}
}


