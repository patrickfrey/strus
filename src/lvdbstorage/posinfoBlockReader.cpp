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
#include "posinfoBlockReader.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

PosinfoBlockReader::PosinfoBlockReader( const PosinfoBlockReader& o)
	:m_db(o.m_db)
	,m_itr(0)
	,m_key(o.m_key)
	,m_keysize(o.m_keysize)
	,m_last_docno(o.m_last_docno)
	,m_posinfoBlock(o.m_posinfoBlock)
{
}

PosinfoBlockReader::PosinfoBlockReader( leveldb::DB* db_, const Index& typeno_, const Index& termno_)
	:m_db(db_)
	,m_itr(0)
	,m_key( (char)DatabaseKey::PosinfoBlockPrefix, typeno_, termno_)
	,m_keysize(0)
	,m_last_docno(0)
{
	m_keysize = m_key.size();
}

PosinfoBlockReader::~PosinfoBlockReader()
{
	if (m_itr) delete m_itr;
}

bool PosinfoBlockReader::extractData()
{
	if (m_itr->Valid()
	&&  m_keysize <= m_itr->key().size()
	&&  0==std::memcmp( m_key.ptr(), m_itr->key().data(), m_keysize))
	{
		const char* ki = m_itr->key().data();
		const char* ke = vi + m_itr->key().size();
		Index docno = unpackIndex( ki, ke);

		const char* vi = m_itr->value().data();
		std::size_t vsize = m_itr->value().size();

		m_posinfoBlock.init( docno, vi, vsize);
		return true;
	}
	else
	{
		m_posinfoBlock.clear();
		return false;
	}
}

const PosinfoBlock* PosinfoBlockReader::readBlock( const Index& docno_)
{
	if (!m_posinfoBlock.empty())
	{
		if (docno_ >= m_last_docno
		&&  docno_ <= m_posinfoBlock.docno())
		{
			return &m_posinfoBlock;
		}
		else if (docno_ == m_posinfoBlock.docno()+1)
		{
			m_itr->Next();
		}
		else
		{
			m_key.resize( m_keysize);
			m_key.addElem( docno_);
			m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
		}
	}
	else
	{
		if (!m_itr)
		{
			leveldb::ReadOptions options;
			options.fill_cache = true;
			m_itr = m_db->NewIterator( options);
		}
		m_key.resize( m_keysize);
		m_key.addElem( docno_);
		m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
	}
	m_last_docno = docno_;
	if (extractData())
	{
		return &m_posinfoBlock;
	}
	else
	{
		return 0;
	}
}

const DocnoBlock* PosinfoBlockReader::readLastBlock()
{
	if (!m_itr)
	{
		leveldb::ReadOptions options;
		options.fill_cache = true;
		m_itr = m_db->NewIterator( options);
	}
	m_key.resize( m_keysize);
	m_key.addPrefix( (char)0xff);
	m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
	if (!m_itr->Valid())
	{
		m_itr->SeekToLast();
		if (!m_itr->Valid()) return 0;
		m_itr->Prev();
	}
	if (extractData())
	{
		return &m_posinfoBlock;
	}
	else
	{
		return 0;
	}
}

