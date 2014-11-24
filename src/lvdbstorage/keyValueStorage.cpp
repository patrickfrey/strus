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
#include "keyValueStorage.hpp"

using namespace strus;

const KeyValueStorage::Value* KeyValueStorage::load( const Key& key)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( m_readOptions, keyslice, &m_curvaluestr);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	m_curvalue.init( m_curvaluestr.c_str(), m_curvaluestr.size());
	return &m_curvalue;
}

void KeyValueStorage::store( const Key& key, const Value& value, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Slice valueslice( value.ptr(), value.size());

	batch.Put( keyslice, valueslice);
}

void KeyValueStorage::dispose( const Key& key, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());

	batch.Delete( keyslice);
}


