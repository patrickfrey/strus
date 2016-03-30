/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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


