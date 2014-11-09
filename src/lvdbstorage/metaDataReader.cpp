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

MetaDataReader::MetaDataReader( leveldb::DB* db_, char varname_)
	:m_db(db_)
	,m_itr(db_->NewIterator( leveldb::ReadOptions()))
	,m_varname(varname_)
	,m_key( (char)DatabaseKey::DocMetaDataPrefix, varname_)
	,m_blockno(0)
	,m_blk(0)
{}

MetaDataReader::~MetaDataReader()
{
	delete m_itr;
}

float MetaDataReader::readValue( const Index& docno_)
{
	Index blockno_ = MetaDataBlock::blockno( docno_);
	std::size_t index_ = MetaDataBlock::index( docno_);

	if (m_blockno == blockno_ && m_blk)
	{
		return m_blk[ index_];
	}
	else if (m_blockno +1 == blockno_ && m_blockno)
	{
		m_blockno = blockno_;
		m_itr->Next();
	}
	else
	{
		m_blockno = blockno_;
		m_key.resize( 2);
		m_key.addElem( m_blockno);
		m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
	}
	if (2 < m_itr->key().size() || 0!=std::memcmp( m_key.ptr(), m_itr->key().data(), 2))
	{
		m_blk = 0;
		m_blockno = 0;
		return 0.0;
	}
	m_blk = reinterpret_cast<const float*>( m_itr->value().data());
	// ... memory is aligned to word length
	unsigned int valuesize = m_itr->value().size();
	if (valuesize != MetaDataBlock::MetaDataBlockSize * sizeof(float))
	{
		m_blk = 0;
		m_blockno = 0;
		throw std::runtime_error( "internal: corrupt metadata block");
	}
	Index nextBlockno = m_key.elem(2);
	if (m_blockno != nextBlockno)
	{
		m_blockno = nextBlockno;
		return 0.0;
	}
	return m_blk[ index_];
}




