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
#include "metaDataReader.hpp"
#include "metaDataBlock.hpp"
#include "databaseKey.hpp"
#include <stdexcept>
#include <cstring>

using namespace strus;

MetaDataBlock* MetaDataReader::readBlockFromDB( leveldb::DB* db, Index blockno, char varname)
{
	DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix, varname, blockno);
	leveldb::Slice constkey( key.ptr(), key.size());
	std::string value;
	value.reserve( MetaDataBlock::MetaDataBlockSize*sizeof(float));
	leveldb::Status status = db->Get( leveldb::ReadOptions(), constkey, &value);

	if (status.IsNotFound())
	{
		return new MetaDataBlock( blockno);
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	if (value.size() != MetaDataBlock::MetaDataBlockSize * sizeof(float))
	{
		throw std::runtime_error( "internal: size of metadata block on disk does not match");
	}
	const float* blk = reinterpret_cast<const float*>( value.c_str());
	return new MetaDataBlock( blockno, blk, MetaDataBlock::MetaDataBlockSize);
}



