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
#include "forwardIndexViewer.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"

using namespace strus;

ForwardIndexViewer::ForwardIndexViewer( Storage* storage_, leveldb::DB* db_, const std::string& type_)
	:m_storage(storage_)
	,m_db(db_)
	,m_itr(0)
	,m_type(type_)
	,m_docno(0)
	,m_typeno(0)
	,m_pos(0)
	,m_keylevel(0)
	,m_keysize_docno(0)
	,m_keysize_typeno(0)
{}

ForwardIndexViewer::~ForwardIndexViewer()
{
	if (m_itr) delete m_itr;
}

void ForwardIndexViewer::buildKey( int level)
{
	switch (level)
	{
		case 0:
			m_key.resize(0);
			m_keysize_docno = 0;
			m_keysize_typeno = 0;
			m_keylevel = 0;
			break;
		case 1:
			m_key.resize(0);
			packIndex( m_key, m_docno);
			m_keysize_docno = m_key.size();
			m_keysize_typeno = 0;
			m_keylevel = 1;
			break;
		case 2:
			if (m_keylevel < 1) buildKey(1);
			m_key.resize( m_keysize_docno);
			packIndex( m_key, m_typeno);
			m_keysize_typeno = m_key.size();
			m_keylevel = 2;
			break;
		case 3:
			if (m_keylevel < 2) buildKey(2);
			m_key.resize( m_keysize_typeno);
			packIndex( m_key, m_pos);
			break;
		default:
			throw std::logic_error("assertion failed in forward index viewer: key level corrupted");
	}
}


void ForwardIndexViewer::initDoc( const Index& docno_)
{
	if (!m_typeno)
	{
		m_typeno = m_storage->keyLookUp( Storage::TermTypePrefix, m_type);
	}
	if (m_docno != docno_)
	{
		buildKey(2);
	}
}


Index ForwardIndexViewer::skipPos( const Index& firstpos_)
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( leveldb::ReadOptions());
	}
	if (m_keylevel < 3)
	{
		if (!m_docno)
		{
			throw std::runtime_error( "cannot seek position without document number defined");
		}
		m_pos = firstpos_;
		buildKey(3);
		m_itr->Seek( leveldb::Slice( m_key.c_str(), m_key.size()));
	}
	else
	{
		if (m_pos == firstpos_ +1)
		{
			m_itr->Next();
		}
		else
		{
			m_itr->Seek( leveldb::Slice( m_key.c_str(), m_key.size()));
		}
	}
	if (m_keysize_typeno < m_itr->key().size()
	&&  0==std::memcmp( m_key.c_str(), m_itr->key().data(), m_keysize_typeno))
	{
		// Extract the current position from the rest of the key and set it:
		const char* ki = m_itr->key().data() + m_keysize_typeno;
		const char* ke = ki + m_itr->key().size();
		return (m_pos = unpackIndex( ki, ke));
	}
	else
	{
		return m_pos = 0;
	}
}


std::string ForwardIndexViewer::fetch()
{
	if (m_keylevel < 3)
	{
		throw std::runtime_error("internal: try to fetch item from forward index with document, term type or position undefined");
	}
	return std::string( m_itr->value().data(), m_itr->value().size());
}


