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
#include "databaseAdapter.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/storagePeerTransactionInterface.hpp"
#include "strus/private/internationalization.hpp"
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "indexPacker.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyMap::clear()
{
	m_map.clear();
}

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
				throw strus::runtime_error( _TXT( "term value undefined for %d (term number map for df)"), mi->first.second);
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

void DocumentFrequencyMap::getWriteBatch(
		DatabaseTransactionInterface* transaction,
		StoragePeerTransactionInterface* peerTransaction,
		const KeyMapInv& termTypeMapInv,
		const KeyMapInv& termValueMapInv)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();

	for (; mi != me; ++mi)
	{
		if (mi->second == 0) continue;

		Index df = DatabaseAdapter_DocFrequency::get(
				m_database, mi->first.first/*typeno*/, mi->first.second/*termno*/);
		if (peerTransaction)
		{
			const char* typestr = termTypeMapInv.get( mi->first.first/*typeno*/);
			const char* termstr = termValueMapInv.get( mi->first.second/*termno*/);
			if (!typestr) throw strus::runtime_error( _TXT( "term type not defined in inverse key map for typeno %d"), mi->first.first);
			if (!termstr) throw strus::runtime_error( _TXT( "term value not defined in inverse key map for termno %d"), mi->first.second);

			peerTransaction->populateDocumentFrequencyChange( typestr, termstr, mi->second, (df==0));
		}
		df += mi->second;
		if (df < 0) throw strus::runtime_error( _TXT( "document frequency got negative"));

		DatabaseAdapter_DocFrequency::store(
				transaction, mi->first.first/*typeno*/, mi->first.second/*termno*/, df);
	}
	m_map.clear();
}



