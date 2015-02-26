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
#ifndef _STRUS_LVDB_METADATA_BLOCK_CACHE_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_BLOCK_CACHE_HPP_INCLUDED
#include "strus/index.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "databaseAdapter.hpp"
#include <utility>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;

class MetaDataBlockCache
{
public:
	MetaDataBlockCache( DatabaseClientInterface* database, const MetaDataDescription& descr_);

	~MetaDataBlockCache(){}

	const MetaDataRecord get( const Index& docno);

	void declareVoid( const Index& blockno);
	void refresh();

private:
	void resetBlock( const Index& blockno);

private:
	enum {
		CacheSize=(1024*1024),				///< size of the cache in blocks
		MaxDocno=(CacheSize*MetaDataBlock::BlockSize)	///< hardcode limit of maximum document number
	};

private:
	DatabaseClientInterface* m_database;
	MetaDataDescription m_descr;
	DatabaseAdapter_DocMetaData m_dbadapter;
	boost::shared_ptr<MetaDataBlock> m_ar[ CacheSize];
	std::vector<unsigned int> m_voidar;
};

}
#endif

