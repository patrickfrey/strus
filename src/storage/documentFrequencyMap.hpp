/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DOCUMENT_FREQUENCY_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_FREQUENCY_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/localStructAllocator.hpp"
#include "documentFrequencyCache.hpp"
#include <cstdlib>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class StatisticsBuilderInterface;
/// \brief Forward declaration
class KeyMapInv;

class DocumentFrequencyMap
{
public:
	DocumentFrequencyMap( DatabaseClientInterface* database_)
		:m_database(database_){}

	void increment( Index typeno, Index termno, Index count=1);
	void decrement( Index typeno, Index termno, Index count=1);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StatisticsBuilderInterface* statisticsBuilder,
			DocumentFrequencyCache::Batch* dfbatch,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv);

	void clear();

private:
	typedef std::pair<Index,Index> Key;
	typedef LocalStructAllocator<std::pair<Key,int> > MapAllocator;
	typedef std::less<Key> MapCompare;
	typedef std::map<Key,int,MapCompare, MapAllocator> Map;

private:
	DatabaseClientInterface* m_database;
	Map m_map;
};

}//namespace
#endif


