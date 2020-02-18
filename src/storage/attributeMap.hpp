/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_ATTRIBUTE_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_ATTRIBUTE_MAP_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "blockKey.hpp"
#include "private/localStructAllocator.hpp"
#include "private/stringMap.hpp"
#include "strus/base/symbolTable.hpp"
#include <vector>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class AttributeMap
{
public:
	explicit AttributeMap( DatabaseClientInterface* database_)
		:m_database(database_){}

	~AttributeMap(){}

	void defineAttribute( const Index& docno, const Index& varno, const std::string& value);
	void deleteAttributes( const Index& docno);
	void deleteAttribute( const Index& docno, const Index& varno);

	void renameNewDocNumbers( const std::map<Index,Index>& renamemap);
	void getWriteBatch( DatabaseTransactionInterface* transaction);

	void clear();

	void swap( AttributeMap& o);

private:
	typedef LocalStructAllocator<std::pair<const BlockKeyIndex,const char*> > MapAllocator;
	typedef std::less<BlockKeyIndex> MapCompare;
	typedef std::map<BlockKeyIndex,const char*,MapCompare,MapAllocator> Map;
	typedef std::vector<BlockKeyIndex> DeleteList;

private:
	DatabaseClientInterface* m_database;
	Map m_map;
	SymbolVector m_strings;
	DeleteList m_deletes;
};

}//namespace
#endif

