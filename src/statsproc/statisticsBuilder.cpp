/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a builder for a statistics message (distributed index)
/// \file statisticsBuilder.cpp

#include "statisticsBuilder.hpp"
#include "statisticsHeader.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utf8.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <arpa/inet.h>
#include <stdint.h>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

StatisticsBuilder::StatisticsBuilder( bool insertInLexicalOrder_, std::size_t maxblocksize_, ErrorBufferInterface* errorhnd)
	:m_insertInLexicalOrder(insertInLexicalOrder_),m_content_consumed(false),m_cnt(0),m_blocksize(0),m_maxblocksize(maxblocksize_),m_errorhnd(errorhnd)
{
	clear();
}

StatisticsBuilder::~StatisticsBuilder()
{}

void StatisticsBuilder::setNofDocumentsInsertedChange(
			int increment)
{
	if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
	{
		m_errorhnd->report( _TXT( "number of documents inserted change value is out of range"));
	}
	else
	{
		StatisticsHeader* hdr = reinterpret_cast<StatisticsHeader*>( const_cast<char*>( m_content.back().c_str()));
		hdr->nofDocumentsInsertedChange = htonl( (uint32_t)(int32_t)increment);
	}
}

#ifdef STRUS_LOWLEVEL_DEBUG
static void printRecord( std::ostream& out, const char* rc, std::size_t rcsize)
{
	static const char HEX[] = "0123456789ABCDEF";
	std::size_t ii = 0;
	for (; ii<rcsize; ++ii)
	{
		if (rc[ii] >= '0' && rc[ii] <= '9')
		{
			out << rc[ ii];
		}
		else if ((rc[ii]|32) >= 'a' && (rc[ii]|32) <= 'z')
		{
			out << rc[ ii];
		}
		else
		{
			out << "[" << HEX[(unsigned char)rc[ ii]/16] << HEX[(unsigned char)rc[ ii]%16] << "]";
		}
	}
	out << std::endl;
}
#endif

void StatisticsBuilder::addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment)
{
	try
	{
		if (m_content.empty())
		{
			clear();
		}
		std::string rec;
		std::size_t termtypesize = std::strlen(termtype);
		std::size_t termvaluesize = std::strlen(termvalue);
		rec.reserve( termtypesize + termvaluesize + 2);
		rec.append( termtype, termtypesize);
		rec.push_back( '\1');
		rec.append( termvalue, termvaluesize);
		if (rec.size() > (uint32_t)std::numeric_limits<int32_t>::max() -64)
		{
			m_errorhnd->report( _TXT( "document frequency change term size is out of range"));
		}
		else
		{
			m_blocksize += termtypesize + termvaluesize;
			if (m_blocksize > m_maxblocksize)
			{
				newContent();
			}
			if (m_insertInLexicalOrder)
			{
		
				addDfChange_final( rec, increment);
			}
			else
			{
				addDfChange_tree( rec, increment);
			}
		}
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder add df change: %s"), *m_errorhnd);
}

void StatisticsBuilder::addDfChange_tree(
		const std::string& key,
		int increment)
{
	bool sign = (increment<0);
	if (sign)
	{
		increment = -increment;
	}
	unsigned int flags = (sign?0x1:0x0);
	unsigned int val = (increment << 1) + flags;
	if (!m_tree.set( key.c_str(), (conotrie::CompactNodeTrie::NodeData)val))
	{
		newContent();
		if (!m_tree.set( key.c_str(), (conotrie::CompactNodeTrie::NodeData)val))
		{
			m_errorhnd->report( _TXT( "feature key is out of range"));
		}
	}
}

void StatisticsBuilder::addDfChange_final(
		const std::string& key,
		int increment)
{
	if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
	{
		m_errorhnd->report( _TXT( "df increment is out of range"));
		return;
	}
	std::string& content = m_content.back();

	std::string pldata;
	char idxbuf[ 32];
	unsigned char flags = 0x0;
	if (increment < 0)
	{
		flags |= 0x1;
		increment = -increment;
	}
	pldata.push_back( flags);
	std::size_t idxpos = utf8encode( idxbuf, (int32_t)increment);
	pldata.append( idxbuf, idxpos);

#ifdef STRUS_LOWLEVEL_DEBUG
	std::size_t itemidx = content.size();
#endif
	std::size_t ii = 0;
	for (; ii<m_lastkey.size() && ii<key.size(); ++ii)
	{
		if (m_lastkey[ii] != key[ ii]) break;
	}
	m_lastkey = key;
	std::size_t commonsize = ii;
	std::size_t restsize = key.size() - ii;

	idxpos = utf8encode( idxbuf, (int32_t)commonsize);
	content.append( idxbuf, idxpos);

	idxpos = utf8encode( idxbuf, (int32_t)(restsize) + pldata.size());
	content.append( idxbuf, idxpos);
	content.append( key.c_str() + commonsize, restsize);
	char* ci = const_cast<char*>( std::strchr( content.c_str() + content.size() - restsize, '\1'));
	if (ci) *ci = '\0';
	content.append( pldata);
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "BLOCK ";
	printRecord( std::cerr, content.c_str() + itemidx, content.size() - itemidx);
#endif
}

bool StatisticsBuilder::fetchMessage( const char*& blk, std::size_t& blksize)
{
	if (m_content.empty())
	{
		clear();
		return false;
	}
	try
	{
		if (!m_insertInLexicalOrder)
		{
			moveTree();
		}
		if (m_content_consumed)
		{
			m_content.pop_front();
			m_content_consumed = false;
		}
		while (!m_content.empty())
		{
			std::string rt = m_content.front();
			StatisticsHeader* hdr = reinterpret_cast<StatisticsHeader*>( const_cast<char*>( rt.c_str()));
			if (rt.size() == sizeof(*hdr) && hdr->empty())
			{
				m_content.pop_front();
				continue;
			}
			m_content_consumed = true;
			blk = m_content.front().c_str();
			blksize = m_content.front().size();
			return true;
		}
		clear();
		blk = 0;
		blksize = 0;
		return false;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error statistics message builder fetch message: %s"), *m_errorhnd, false);
}

void StatisticsBuilder::clear()
{
	m_content.clear();
	m_tree.clear();
	m_cnt = 0;
	m_content_consumed = false;
	newContent();
}

void StatisticsBuilder::moveTree()
{
	conotrie::CompactNodeTrie::const_iterator ti = m_tree.begin(), te = m_tree.end();
	if (ti == te) return;
	for (; ti != te; ++ti)
	{
		unsigned int val = (unsigned int)ti.data();
		int increment = val >> 1;
		bool sign = ((val&0x1) != 0);
		if (sign)
		{
			increment = -increment;
		}
		addDfChange_final( ti.key(), increment);
	}
	m_tree.reset();
}

void StatisticsBuilder::newContent()
{
	if (!m_insertInLexicalOrder)
	{
		moveTree();
	}
	StatisticsHeader hdr;
	if (m_maxblocksize < (1<<20))
	{
		std::string blk;
		blk.reserve( m_maxblocksize);
		blk.append( (char*)&hdr, sizeof(hdr));
		m_content.push_back( blk);
	}
	else
	{
		m_content.push_back( std::string( (char*)&hdr, sizeof(hdr)));
	}
	m_cnt += 1;
	m_blocksize = 0;
	m_lastkey.clear();
}

void StatisticsBuilder::start()
{
	try
	{
		m_cnt = 0;
		newContent();
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder start: %s"), *m_errorhnd);
}

void StatisticsBuilder::rollback()
{
	try
	{
		m_tree.clear();
		for (; m_cnt > 0; --m_cnt)
		{
			m_content.pop_back();
		}
		if (m_content.empty())
		{
			newContent();
		}
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder rollback: %s"), *m_errorhnd);
}


