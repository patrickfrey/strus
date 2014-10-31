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
#include "iterator.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>

using namespace strus;

Iterator::Iterator( leveldb::DB* db_, Index termtypeno, Index termvalueno)
	:m_db(db_),m_docno(0),m_itr(0),m_frequency(0),m_posno(0),m_positr(0),m_posend(0)
{
	m_key.push_back( (char)Storage::LocationPrefix);
	packIndex( m_key, termtypeno);
	packIndex( m_key, termvalueno);
	m_keysize = m_key.size();
	m_featureid.append( m_key.c_str()+1, m_keysize-1);
}

Iterator::Iterator( const Iterator& o)
	:IteratorInterface(o)
	,m_db(o.m_db)
	,m_key(o.m_key)
	,m_keysize(o.m_keysize)
	,m_docno(0)
	,m_itr(0)
	,m_frequency(o.m_frequency)
	,m_posno(0)
	,m_positr(0)
	,m_posend(0)
	,m_featureid(o.m_featureid)
{}

Iterator::~Iterator()
{
	if (m_itr) delete m_itr;
}


Index Iterator::skipDoc( const Index& docno)
{
	if (m_itr && m_docno +1 == docno)
	{
		return getNextTermDoc();
	}
	else
	{
		return getFirstTermDoc( docno);
	}
}

Index Iterator::skipPos( const Index& firstpos)
{
	if (m_posno >= firstpos)
	{
		if (m_posno == firstpos && firstpos != 0)
		{
			return m_posno;
		}
		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();
		m_positr = skipIndex( m_positr, m_posend); //... skip ff
	}
	unsigned int ofs = (m_posend - m_positr) >> 1;
	if (ofs > firstpos - m_posno)
	{
		ofs = (firstpos - m_posno) >> 4;
	}
	while (ofs >= 6)
	{
		const char* skipitr = strus::nextPackedIndexPos( m_positr, m_positr + ofs, m_posend);
		if (skipitr != m_posend)
		{
			Index nextpos = unpackIndex( skipitr, m_posend);
			if (nextpos <= firstpos)
			{
				m_posno = nextpos;
				m_positr = skipitr;
				if (nextpos == firstpos)
				{
					return m_posno;
				}
				else
				{
					ofs = (m_posend - m_positr) >> 1;
					if (ofs > firstpos - m_posno)
					{
						ofs = (firstpos - m_posno) >> 4;
					}
					continue;
				}
			}
			else
			{
				ofs >>= 1;
			}
		}
	}
	while (m_positr < m_posend && (firstpos > m_posno || !m_posno))
	{
		// Get the next position:
		m_posno = unpackIndex( m_positr, m_posend);
	}
	if (firstpos > m_posno)
	{
		return 0;
	}
	return m_posno;
}

Index Iterator::extractMatchData()
{
	if (m_keysize < m_itr->key().size() && 0==std::memcmp( m_key.c_str(), m_itr->key().data(), m_keysize))
	{
		// Init the term weight and the iterators on the term occurrencies:
		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();
		m_frequency = (unsigned int)unpackIndex( m_positr, m_posend);

		// Extract the next matching document number from the rest of the key and return it:
		const char* ki = m_itr->key().data() + m_keysize;
		const char* ke = ki + m_itr->key().size();
		m_docno = unpackIndex( ki, ke);
		return m_docno;
	}
	else
	{
		delete m_itr;
		m_docno = 0;
		m_itr = 0;
		m_frequency = 0;
		m_posno = 0;
		m_positr = 0;
		m_posend = 0;
		return 0;
	}
}

Index Iterator::getNextTermDoc()
{
	m_itr->Next();
	return extractMatchData();
}

Index Iterator::getFirstTermDoc( const Index& docno)
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( leveldb::ReadOptions());
	}
	m_key.resize( m_keysize);
	packIndex( m_key, docno);
	m_itr->Seek( leveldb::Slice( m_key.c_str(), m_key.size()));

	return extractMatchData();
}


