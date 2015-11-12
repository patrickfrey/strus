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
#include "private/stringMap.hpp"
#include "private/internationalization.hpp"

using namespace strus;

StringMapKeyBlock::StringMapKeyBlock( std::size_t blksize_)
	:m_blk((char*)std::calloc(blksize_,1)),m_blksize(blksize_),m_blkpos(0)
{
	if (!m_blk) throw std::bad_alloc();
}

StringMapKeyBlock::StringMapKeyBlock( const StringMapKeyBlock& o)
	:m_blk((char*)std::malloc(o.m_blksize)),m_blksize(o.m_blksize),m_blkpos(o.m_blkpos)
{
	if (!m_blk) throw std::bad_alloc();
	std::memcpy( m_blk, o.m_blk, o.m_blksize);
}

StringMapKeyBlock::~StringMapKeyBlock()
{
	std::free( m_blk);
}

const char* StringMapKeyBlock::allocKey( const std::string& key)
{
	const char* rt = m_blk + m_blkpos;
	if (key.size() > m_blksize || key.size() + m_blkpos + 1 > m_blksize) return 0;
	std::memcpy( m_blk + m_blkpos, key.c_str(), key.size()+1);
	m_blkpos += key.size()+1;
	return rt;
}

const char* StringMapKeyBlock::allocKey( const char* key, std::size_t keylen)
{
	const char* rt = m_blk + m_blkpos;
	if (keylen > m_blksize || keylen + m_blkpos + 1 > m_blksize) return 0;
	std::memcpy( m_blk + m_blkpos, key, keylen+1);
	m_blkpos += keylen+1;
	return rt;
}

const char* StringMapKeyBlockList::allocKey( const char* key, std::size_t keylen)
{
	const char* rt;
	if (m_ar.empty())
	{
		if (keylen > StringMapKeyBlock::DefaultSize)
		{
			m_ar.push_front( StringMapKeyBlock( keylen+1));
			rt = m_ar.front().allocKey( key, keylen);
		}
		else
		{
			m_ar.push_back( StringMapKeyBlock());
			rt = m_ar.back().allocKey( key, keylen);
		}
	}
	else
	{
		rt = m_ar.back().allocKey( key, keylen);
		if (!rt)
		{
			if (keylen > StringMapKeyBlock::DefaultSize)
			{
				m_ar.push_front( StringMapKeyBlock( keylen+1));
				rt = m_ar.front().allocKey( key, keylen);
			}
			else
			{
				m_ar.push_back( StringMapKeyBlock());
				rt = m_ar.back().allocKey( key, keylen);
			}
		}
	}
	if (!rt) throw std::bad_alloc();
	return rt;
}


void StringMapKeyBlockList::clear()
{
	m_ar.clear();
}


