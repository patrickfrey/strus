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
/// \brief Implementation of the interface for a viewer of a message received from a peer with some statistics (distributed index)
/// \file peerMessageViewer.cpp
#include "peerMessageViewer.hpp"
#include "private/internationalization.hpp"
#include "private/utf8.hpp"

using namespace strus;

PeerMessageViewer::PeerMessageViewer( const char* peermsgptr, std::size_t peermsgsize)
	:m_hdr(reinterpret_cast<const PeerMessageHeader*>(peermsgptr))
	,m_peermsgptr(peermsgptr + sizeof(PeerMessageHeader))
	,m_peermsgitr(peermsgptr + sizeof(PeerMessageHeader))
	,m_peermsgend(peermsgptr + peermsgsize)
	,m_peermsgsize(peermsgsize - sizeof(PeerMessageHeader))
{
	if (m_peermsgitr > m_peermsgend)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (message size)"));
	}
}

PeerMessageViewer::~PeerMessageViewer(){}

int PeerMessageViewer::nofDocumentsInsertedChange()
{
	return m_hdr->nofDocumentsInsertedChange;
}

bool PeerMessageViewer::nextDfChange( DocumentFrequencyChange& rec)
{
	if (m_peermsgitr == m_peermsgend) return false;
	std::size_t chlen = utf8charlen( *m_peermsgitr);
	if (m_peermsgitr + chlen + 1 >= m_peermsgend)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (unexpected end of message [1])"));
	}
	int32_t commonbytes = utf8decode( m_peermsgitr, chlen);
	if (commonbytes < 0)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (corrupt block [1])"));
	}
	m_peermsgitr += chlen;
	chlen = utf8charlen( *m_peermsgitr);
	if (m_peermsgitr + chlen + 1 >= m_peermsgend)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (unexpected end of message [2])"));
	}
	int32_t restlen = utf8decode( m_peermsgitr, chlen);
	m_peermsgitr += chlen;
	if (restlen < 0)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (corrupt block [2])"));
	}
	if (m_peermsgitr + restlen > m_peermsgend)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (unexpected end of message [3])"));
	}
	m_msg.resize( commonbytes);
	m_msg.append( m_peermsgitr, restlen);
	m_peermsgitr += restlen;

	char const* mi = m_msg.c_str();
	rec.type = mi;
	std::size_t typesize = std::strlen( mi);
	mi += typesize + 1;
	rec.value = mi;
	std::size_t valuesize = std::strlen( mi);
	mi += valuesize + 1;
	std::size_t midx = typesize + valuesize + 2;
	if (midx + 1 >= m_msg.size())
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (corrupt message record [2])"));
	}
	unsigned char flags = *mi;
	if (flags >= 0x4)
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (corrupt message record [3])"));
	}
	++mi;
	++midx;
	chlen = utf8charlen( *mi);
	rec.increment = utf8decode( mi, chlen);
	if ((flags & 0x2) != 0)
	{
		rec.increment = -rec.increment;
	}
	rec.isnew = ((flags & 0x1) != 0);

	mi += chlen;
	midx += chlen;
	if (midx > m_msg.size())
	{
		throw strus::runtime_error( _TXT( "got illegal message from peer (corrupt message record)"));
	}
	return true;
}

