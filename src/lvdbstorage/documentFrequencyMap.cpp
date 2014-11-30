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
#include "documentFrequencyMap.hpp"
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include <cstdlib>

using namespace strus;

void DocumentFrequencyMap::increment( Index typeno, Index termno)
{
	Key key( typeno, termno);
	m_map[ key] += 1;
}

void DocumentFrequencyMap::decrement( Index typeno, Index termno)
{
	Key key( typeno, termno);
	m_map[ key] -= 1;
}

void DocumentFrequencyMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();

	for (; mi != me; ++mi)
	{
		std::string keystr;
		keystr.push_back( DatabaseKey::DocFrequencyPrefix);
		packIndex( keystr, mi->first.first);	// ... [typeno]
		packIndex( keystr, mi->first.second);	// ... [valueno]

		leveldb::Slice keyslice( keystr.c_str(), keystr.size());
		Index df;
		std::string value;
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
		if (status.IsNotFound() || value.empty())
		{
			df = mi->second;
			if (df < 0) throw std::runtime_error("internal: decrementing document frequency of an undefined term");
		}
		else
		{
			if (!status.ok())
			{
				throw std::runtime_error( status.ToString());
			}
			char const* cc = value.c_str();
			char const* ee = value.c_str() + value.size();
			df = mi->second + unpackIndex( cc, ee);
			if (df < 0) throw std::runtime_error("internal: document frequency got negative");
		}
		enum {MaxValueSize = sizeof(Index)*4};
		char valuebuf[ MaxValueSize];
		std::size_t valuepos = 0;
		packIndex( valuebuf, valuepos, MaxValueSize, df);
		leveldb::Slice valueslice( valuebuf, valuepos);

		batch.Put( keyslice, valueslice);
	}
	m_map.clear();
}



