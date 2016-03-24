/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "private/localStructAllocator.hpp"
#include "strus/numericVariant.hpp"
#include <cstdlib>
#include <vector>

namespace strus {
/// \brief Forward declaration
class MetaDataBlockCache;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;


class MetaDataMap
{
public:
	MetaDataMap( DatabaseClientInterface* database_, const MetaDataDescription* descr_)
		:m_database(database_),m_descr(descr_){}
	~MetaDataMap();

	void defineMetaData( Index docno, const std::string& varname, const NumericVariant& value);
	void deleteMetaData( Index docno);
	void deleteMetaData( Index docno, const std::string& varname);

	void renameNewDocNumbers( const std::map<Index,Index>& renamemap);
	void getWriteBatch( DatabaseTransactionInterface* transaction, std::vector<Index>& cacheRefreshList);
	void rewriteMetaData(
			const MetaDataDescription::TranslationMap& trmap,
			const MetaDataDescription& newDescr,
			DatabaseTransactionInterface* transaction);

private:
	MetaDataRecord getRecord( Index docno);
	void clear();

private:
	typedef std::pair<Index,Index> MetaDataKey;
	typedef LocalStructAllocator<std::pair<MetaDataKey,NumericVariant> > MapAllocator;
	typedef std::less<MetaDataKey> MapCompare;
	typedef std::map<MetaDataKey,NumericVariant,MapCompare,MapAllocator> Map;

private:
	DatabaseClientInterface* m_database;
	const MetaDataDescription* m_descr;
	Map m_map;
};

}
#endif

