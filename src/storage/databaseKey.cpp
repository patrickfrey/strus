/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseKey::DatabaseKey( char prefix)
{
	m_buf[ 0] = (char)prefix;
	m_size = 1;
}

DatabaseKey::DatabaseKey( char prefix, const std::string& varname)
{
	if (varname.size() >= MaxKeySize-1)
	{
		throw strus::runtime_error( _TXT( "database variable key out of range '%s'"), varname.c_str());
	}
	m_buf[ 0] = prefix;
	std::memcpy( m_buf+1, varname.c_str(), varname.size());
	m_size = varname.size()+1;
}

DatabaseKey::DatabaseKey( char prefix, const BlockKey& blkkey, const Index& elemidx)
{
	m_buf[ 0] = (char)prefix;
	m_size = 1;
	Index elem1 = blkkey.elem(1);
	Index elem2 = blkkey.elem(2);
	if (elem1) addElem( elem1);
	if (elem2) addElem( elem2);
	if (elemidx) addElem( elemidx);
}

DatabaseKey::DatabaseKey( char prefix, const Index& idx)
{
	m_buf[ 0] = prefix;
	m_size = 1;
	addElem( idx);
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

void DatabaseKey::addElem( const std::string& var)
{
	if (m_size + var.size() >= MaxKeySize-1)
	{
		throw strus::runtime_error( _TXT( "database key exceeds maximum size allowed: '%s'"), var.c_str());
	}
	std::memcpy( m_buf+m_size, var.c_str(), var.size());
	m_size += var.size();
}

void DatabaseKey::addPrefix( char prefix)
{
	if (m_size == MaxKeySize) throw strus::runtime_error( _TXT( "static buffer overflow (%s)"), __FUNCTION__);
	m_buf[ m_size++] = prefix;
}

void DatabaseKey::resize( std::size_t n)
{
	if (n > m_size) throw strus::runtime_error( "%s", _TXT( "resize in database key only allowed as shrinking of size"));
	m_size = n;
}


