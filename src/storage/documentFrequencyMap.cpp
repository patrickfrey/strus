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
#include "documentFrequencyMap.hpp"
#include "databaseKey.hpp"
#include "databaseRecord.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "keyMap.hpp"
#include "indexPacker.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyMap::increment( Index typeno, Index termno, Index count)
{
	Key key( typeno, termno);
	m_map[ key] += count;
}

void DocumentFrequencyMap::decrement( Index typeno, Index termno, Index count)
{
	Key key( typeno, termno);
	m_map[ key] -= count;
}

void DocumentFrequencyMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
{
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		if (KeyMap::isUnknown( mi->first.second))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( mi->first.second);
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (term number map for df)");
			}
			Key newkey( mi->first.first, ri->second);
			m_map[ newkey] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void DocumentFrequencyMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();

	for (; mi != me; ++mi)
	{
		if (mi->second == 0) continue;

		Index df = DatabaseRecord_DocFrequency::get(
				m_database, mi->first.first/*typeno*/, mi->first.second/*termno*/);
		df += mi->second;
		if (df < 0) throw std::runtime_error( "internal: document frequency got negative");

		DatabaseRecord_DocFrequency::store(
				transaction, mi->first.first/*typeno*/, mi->first.second/*termno*/, df);
	}
	m_map.clear();
}


