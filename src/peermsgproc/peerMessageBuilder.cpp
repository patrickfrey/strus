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

PeerMessageBuilder::PeerMessageBuilder( bool insertInLexicalOrder_)
	:m_insertInLexicalOrder(insertInLexicalOrder_),m_lastmsgpos(0)
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
		throw strus::runtime_error( _TXT( "number of documents inserted change value is out of range"));
	}
	PeerMessageHeader* hdr = reinterpret_cast<PeerMessageHeader*>( const_cast<char*>( m_content.c_str()));
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
	if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
	{
		throw strus::runtime_error( _TXT( "document frequency change value is out of range"));
	}
	char idxbuf[ 32];
	std::string rec;
	rec.append( termtype, std::strlen(termtype));
	rec.push_back( '\0');
	rec.append( termvalue, std::strlen(termvalue));
	rec.push_back( '\0');
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
	rec.push_back( flags);
	std::size_t idxpos = utf8encode( idxbuf, (int32_t)increment);
	rec.append( idxbuf, idxpos);
	if (rec.size() > (uint32_t)std::numeric_limits<int32_t>::max())
	{
		throw strus::runtime_error( _TXT( "document frequency change term size is out of range"));
	}
	const char* lastmsgptr = m_content.c_str() + m_lastmsgpos;
	std::size_t lastmsgsize = m_content.size() - m_lastmsgpos;
#ifdef STRUS_LOWLEVEL_DEBUG
	std::size_t itemidx = m_content.size();
#endif
	std::size_t ii = 0;
	for (; ii<lastmsgsize && ii<rec.size(); ++ii)
	{
		if (lastmsgptr[ii] != rec[ ii]) break;
	}
	idxpos = utf8encode( idxbuf, (int32_t)ii);
	m_content.append( idxbuf, idxpos);
	idxpos = utf8encode( idxbuf, (int32_t)(rec.size() - ii));
	m_content.append( idxbuf, idxpos);

	m_lastmsgpos = m_content.size();
	m_content.append( rec.c_str() + ii, rec.size() - ii);
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "BLOCK ";
	printRecord( std::cerr, m_content.c_str() + itemidx, m_content.size() - itemidx);
#endif
}

void PeerMessageBuilder::fetchMessage( const char*& msg, std::size_t& msgsize)
{
	msg = m_content.c_str();
	msgsize = m_content.size();
}

void PeerMessageBuilder::clear()
{
	PeerMessageHeader hdr;
	m_content.clear();
	m_content.append( (char*)&hdr, sizeof(hdr));
	m_lastmsgpos = sizeof(hdr);
}

void PeerMessageBuilder::start()
{
	m_stk.push_back( State( m_lastmsgpos, m_content.size()));
}

void PeerMessageBuilder::rollback()
{
	if (m_stk.empty()) throw strus::runtime_error( _TXT( "logic error: calling rollback without start"));
	m_lastmsgpos = m_stk.back().lastmsgpos;
	m_content.resize( m_stk.back().contentsize);
}


