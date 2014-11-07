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
#include "metaDataBlock.hpp"
#include "databaseKey.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

MetaDataBlock::MetaDataBlock( leveldb::DB* db_, Index blockno_, char varname_)
	:m_db(db_),m_blockno(blockno_),m_varname(varname_)
{
	std::memset( m_blk, 0, MetaDataBlockSize);
	readBlockFromDB();
}

MetaDataBlock::MetaDataBlock( const MetaDataBlock& o)
	:m_db(o.m_db),m_blockno(o.m_blockno),m_varname(o.m_varname)
{
	std::memcpy( m_blk, o.m_blk, MetaDataBlockSize);
}

void MetaDataBlock::setValue( Index docno, float value)
{
	if (blockno(docno) != m_blockno)
	{
		throw std::runtime_error( "internal: illegal write access on block (wrong block number)");
	}
	m_blk[ index(docno)] = value;
}

float MetaDataBlock::getValue( Index docno) const
{
	if (blockno( docno) != m_blockno)
	{
		throw std::runtime_error( "internal: illegal read access on block (wrong block number)");
	}
	return m_blk[ docno & MetaDataBlockMask];
}

void MetaDataBlock::readBlockFromDB()
{
	DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix, m_varname, m_blockno);

	leveldb::Slice constkey( key.ptr(), key.size());
	std::string value;
	value.reserve( sizeof(m_blk));
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);

	if (status.IsNotFound())
	{
		std::memset( m_blk, 0, sizeof(m_blk));
		return;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	if (value.size() != sizeof(m_blk))
	{
		throw std::runtime_error( "internal: size of metadata block on disk does not match");
	}
	std::memcpy( m_blk, value.c_str(), value.size());
	if (value.size() < sizeof(m_blk))
	{
		std::memset( m_blk + value.size(), 0, sizeof(m_blk) - value.size());
	}
}

void MetaDataBlock::addToBatch( leveldb::WriteBatch& batch)
{
	DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix, m_varname, m_blockno);

	leveldb::Slice keyslice( key.ptr(), key.size());
	leveldb::Slice valueslice( (const char*)m_blk, sizeof(m_blk));
	batch.Put( keyslice, valueslice);
}






