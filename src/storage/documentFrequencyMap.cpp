/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "documentFrequencyMap.hpp"
#include "databaseAdapter.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "private/internationalization.hpp"
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "indexPacker.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyMap::clear()
{
	m_map.clear();
}

void DocumentFrequencyMap::increment( Index typeno, Index termno, int count)
{
	Key key( typeno, termno);
	m_map[ key] += count;
}

void DocumentFrequencyMap::decrement( Index typeno, Index termno, int count)
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
		StatisticsBuilderInterface* statisticsBuilder,
		DocumentFrequencyCache::Batch* dfbatch,
		const KeyMapInv& termTypeMapInv,
		const KeyMapInv& termValueMapInv)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();

	for (; mi != me; ++mi)
	{
		if (mi->second == 0) continue;

		if (statisticsBuilder)
		{
			const char* typestr = termTypeMapInv.get( mi->first.first/*typeno*/);
			const char* termstr = termValueMapInv.get( mi->first.second/*termno*/);
			if (!typestr) throw strus::runtime_error( _TXT( "term type not defined in inverse key map for typeno %d"), mi->first.first);
			if (!termstr) throw strus::runtime_error( _TXT( "term value not defined in inverse key map for termno %d"), mi->first.second);

			statisticsBuilder->addDfChange( typestr, termstr, mi->second);
		}
		Index df = DatabaseAdapter_DocFrequency::get(
				m_database, mi->first.first/*typeno*/, mi->first.second/*termno*/);
		df += mi->second;
		if (df < 0) throw strus::runtime_error( _TXT( "document frequency got negative: %d (%d)"), (int)df, (int)mi->second);

		DatabaseAdapter_DocFrequency::store(
				transaction, mi->first.first/*typeno*/, mi->first.second/*termno*/, df);
		if (dfbatch)
		{
			dfbatch->put( mi->first.first/*typeno*/, mi->first.second/*termno*/, mi->second);
		}
	}
	// Clear maps:
	clear();
}



