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
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseKey::DatabaseKey( char prefix)
{
	m_buf[ 0] = prefix;
	m_size = 1;
}

DatabaseKey::DatabaseKey( char prefix, const Index& idx)
{
	m_buf[ 0] = prefix;
	m_size = 1;
	addElem( idx);
}

DatabaseKey::DatabaseKey( char prefix, char prefix2, const Index& idx)
{
	m_buf[ 0] = prefix;
	m_buf[ 1] = prefix2;
	m_size = 2;
	addElem( idx);
}

DatabaseKey::DatabaseKey( char prefix, const Index& idx, char prefix2)
{
	m_buf[ 0] = prefix;
	m_size = 1;
	addElem( idx);
	addPrefix( prefix2);
}

DatabaseKey::DatabaseKey( char prefix, const Index& idx, const Index& idx2)
{
	m_buf[ 0] = prefix;
	m_size = 1;
	addElem( idx);
	addElem( idx2);
}

DatabaseKey::DatabaseKey( const DatabaseKey& o)
	:m_size(o.m_size)
{
	std::memcpy( m_buf, o.m_buf, sizeof(m_buf));
}

void DatabaseKey::addElem( const Index& index)
{
	packIndex( m_buf, m_size, MaxKeySize, index);
}

Index DatabaseKey::elem( std::size_t pos) const
{
	if (pos >= m_size) throw std::runtime_error("internal: illegal element access in key");
	const char* pp = m_buf + pos;
	return unpackIndex( pp, m_buf + m_size);
}

void DatabaseKey::addPrefix( char prefix)
{
	if (m_size == MaxKeySize) throw std::runtime_error("static buffer overflow");
	m_buf[ m_size++] = prefix;
}

void DatabaseKey::resize( std::size_t n)
{
	if (n > m_size) throw std::runtime_error("internal: resize in database key only allowed as shrinking of size");
	m_size = n;
}


