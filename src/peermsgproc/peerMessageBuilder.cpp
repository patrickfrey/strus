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
/// \brief Interface for a builder for a message sent to peer(s) to populate some statistics (distributed index)
/// \file peerMessageBuilder.cpp

#include "peerMessageBuilder.hpp"
#include "peerMessageHeader.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utf8.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <arpa/inet.h>
#include <stdint.h>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

PeerMessageBuilder::PeerMessageBuilder( bool insertInLexicalOrder_, std::size_t maxblocksize_, ErrorBufferInterface* errorhnd)
	:m_insertInLexicalOrder(insertInLexicalOrder_),m_content_consumed(false),m_cnt(0),m_blocksize(0),m_maxblocksize(maxblocksize_),m_errorhnd(errorhnd)
{
	clear();
}

PeerMessageBuilder::~PeerMessageBuilder()
{}

void PeerMessageBuilder::setNofDocumentsInsertedChange(
			int increment)
{
	if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
	{
		m_errorhnd->report( _TXT( "number of documents inserted change value is out of range"));
	}
	PeerMessageHeader* hdr = reinterpret_cast<PeerMessageHeader*>( const_cast<char*>( m_content.back().c_str()));
	hdr->nofDocumentsInsertedChange = htonl( (uint32_t)(int32_t)increment);
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

void PeerMessageBuilder::addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment,
			bool isnew)
{
	try
	{
		if (m_content.empty()) clear();
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
		m_blocksize += termtypesize + termvaluesize;
		if (m_blocksize > m_maxblocksize)
		{
			newContent();
		}
		if (m_insertInLexicalOrder)
		{
	
			addDfChange_final( rec, increment, isnew);
		}
		else
		{
			addDfChange_tree( rec, increment, isnew);
		}
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT( "out of memory in peer message builder"));
	}
}

void PeerMessageBuilder::addDfChange_tree(
		const std::string& key,
		int increment,
		bool isnew)
{
	bool sign = (increment<0);
	if (sign)
	{
		increment = -increment;
	}
	unsigned int flags = (isnew?0x1:0x0)|(sign?0x2:0x0);
	unsigned int val = (increment << 2) + flags;
	if (!m_tree.set( key.c_str(), (conotrie::CompactNodeTrie::NodeData)val))
	{
		newContent();
		if (!m_tree.set( key.c_str(), (conotrie::CompactNodeTrie::NodeData)val))
		{
			m_errorhnd->report( _TXT( "feature key is out of range"));
		}
	}
}

void PeerMessageBuilder::addDfChange_final(
		const std::string& key,
		int increment,
		bool isnew)
{
	try
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
			flags |= 0x2;
			increment = -increment;
		}
		if (isnew)
		{
			flags |= 0x1;
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
		std::size_t restsize = key.size() - ii + 1;
	
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
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT( "out of memory in peer message builder"));
	}
}

bool PeerMessageBuilder::fetchMessage( const char*& blk, std::size_t& blksize)
{
	if (m_content.empty()) return false;
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
			PeerMessageHeader* hdr = reinterpret_cast<PeerMessageHeader*>( const_cast<char*>( rt.c_str()));
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
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT( "out of memory in peer message builder"));
		return false;
	}
}

void PeerMessageBuilder::clear()
{
	m_content.clear();
	m_tree.clear();
	m_cnt = 0;
	m_content_consumed = false;
	newContent();
}

void PeerMessageBuilder::moveTree()
{
	conotrie::CompactNodeTrie::const_iterator ti = m_tree.begin(), te = m_tree.end();
	if (ti == te) return;
	for (; ti != te; ++ti)
	{
		unsigned int val = (unsigned int)ti.data();
		int increment = val >> 2;
		bool sign = ((val&0x2) != 0);
		bool isnew = ((val&0x1) != 0);
		if (sign)
		{
			increment = -increment;
		}
		addDfChange_final( ti.key(), increment, isnew);
	}
	m_tree.reset();
}

void PeerMessageBuilder::newContent()
{
	if (!m_insertInLexicalOrder)
	{
		moveTree();
	}
	PeerMessageHeader hdr;
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

void PeerMessageBuilder::start()
{
	try
	{
		m_cnt = 0;
		newContent();
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT( "out of memory in peer message builder"));
	}
}

void PeerMessageBuilder::rollback()
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
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT( "out of memory in peer message builder"));
	}
}


