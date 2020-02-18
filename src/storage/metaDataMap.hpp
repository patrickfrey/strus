/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_METADATA_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_BLOCK_MAP_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/base/shared_ptr.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataBlockCache.hpp"
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
	MetaDataMap( DatabaseClientInterface* database_, const strus::shared_ptr<MetaDataBlockCache>& mdcache_)
		:m_database(database_),m_metaDataBlockCache(mdcache_),m_map()
	{
		m_descr = &m_metaDataBlockCache->descr();
	}
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

	void clear();
	void reset( const strus::shared_ptr<MetaDataBlockCache>& mdcache_);

	const MetaDataDescription* description() const
	{
		return m_descr;
	}
	strus::shared_ptr<MetaDataBlockCache> metaDataBlockCache() const
	{
		return m_metaDataBlockCache;
	}

private:
	MetaDataRecord getRecord( Index docno);

private:
	typedef std::pair<Index,Index> MetaDataKey;
	typedef LocalStructAllocator<std::pair<const MetaDataKey,NumericVariant> > MapAllocator;
	typedef std::less<MetaDataKey> MapCompare;
	typedef std::map<MetaDataKey,NumericVariant,MapCompare,MapAllocator> Map;

private:
	DatabaseClientInterface* m_database;
	strus::shared_ptr<MetaDataBlockCache> m_metaDataBlockCache;
	const MetaDataDescription* m_descr;
	Map m_map;
};

}
#endif

