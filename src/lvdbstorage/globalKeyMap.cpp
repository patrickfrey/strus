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
#include "globalKeyMap.hpp"
#include "indexPacker.hpp"

using namespace strus;

Index GlobalKeyMap::lookUp( DatabaseKey::KeyPrefix prefix, const std::string& key) const
{
	std::string keystr;
	std::string value;
	keystr.push_back( (char)prefix);
	keystr.append( key);

	leveldb::ReadOptions options;
	options.fill_cache = false;

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( options, keyslice, &value);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

Index GlobalKeyMap::getOrCreate( DatabaseKey::KeyPrefix prefix, const std::string& key, Index& valuecnt)
{
	std::string keystr;
	std::string value;
	keystr.push_back( (char)prefix);
	keystr.append( key);

	leveldb::ReadOptions options;
	options.fill_cache = false;

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( options, keyslice, &value);

	if (status.IsNotFound())
	{
		boost::mutex::scoped_lock( m_mutex);
		Map::const_iterator ki = m_map.find( keystr);
		if (ki != m_map.end())
		{
			return ki->second;
		}
		return m_map[ keystr] = valuecnt++;
	}
	else if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	else
	{
		const char* cc = value.c_str();
		return unpackIndex( cc, cc + value.size());
	}
}

void GlobalKeyMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		leveldb::Slice keyslice( mi->first.c_str(), mi->first.size());
		std::string valuestr;
		packIndex( valuestr, mi->second);
		leveldb::Slice valueslice( valuestr.c_str(), valuestr.size());
		batch.Put( keyslice, valueslice);
	}
	m_map.clear();
}


