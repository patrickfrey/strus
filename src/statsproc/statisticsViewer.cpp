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
/// \brief Implementation of the interface for a viewer of a statistics message received (distributed index)
/// \file statisticsViewer.cpp
#include "statisticsViewer.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utf8.hpp"
#include <arpa/inet.h>

using namespace strus;

StatisticsViewer::StatisticsViewer( const char* msgptr, std::size_t msgsize, ErrorBufferInterface* errorhnd_)
	:m_hdr(reinterpret_cast<const StatisticsHeader*>(msgptr))
	,m_msgptr(msgptr + sizeof(StatisticsHeader))
	,m_msgitr(msgptr + sizeof(StatisticsHeader))
	,m_msgend(msgptr + msgsize)
	,m_msgsize(msgsize - sizeof(StatisticsHeader))
	,m_errorhnd(errorhnd_)
{
	if (m_msgitr > m_msgend)
	{
		throw strus::runtime_error( _TXT( "got illegal message from  (message size)"));
	}
}

StatisticsViewer::~StatisticsViewer(){}

int StatisticsViewer::nofDocumentsInsertedChange()
{
	return ntohl( (int32_t)(uint32_t)m_hdr->nofDocumentsInsertedChange);
}

bool StatisticsViewer::nextDfChange( DocumentFrequencyChange& rec)
{
	try
	{
		if (m_msgitr == m_msgend) return false;
		std::size_t chlen = utf8charlen( *m_msgitr);
		if (m_msgitr + chlen + 1 >= m_msgend)
		{
			throw strus::runtime_error( _TXT( "got illegal statistics message (unexpected end of message [1])"));
		}
		int32_t commonbytes = utf8decode( m_msgitr, chlen);
		if (commonbytes < 0)
		{
			throw strus::runtime_error( _TXT( "got illegal statistics message (corrupt block [1])"));
		}
		m_msgitr += chlen;
		chlen = utf8charlen( *m_msgitr);
		if (m_msgitr + chlen + 1 >= m_msgend)
		{
			throw strus::runtime_error( _TXT( "got illegal message from  (unexpected end of message [2])"));
		}
		int32_t restlen = utf8decode( m_msgitr, chlen);
		m_msgitr += chlen;
		if (restlen < 0)
		{
			throw strus::runtime_error( _TXT( "got illegal message from  (corrupt block [2])"));
		}
		if (m_msgitr + restlen > m_msgend)
		{
			throw strus::runtime_error( _TXT( "got illegal message from  (unexpected end of message [3])"));
		}
		m_msg.resize( commonbytes);
		const char* msg_itr = m_msgitr;
		m_msgitr += restlen;
		char const* df_itr = utf8prev( m_msgitr);
		char const* flags_itr = utf8prev( df_itr);
		if (flags_itr < msg_itr)
		{
			throw strus::runtime_error( _TXT( "got illegal message from  (corrupt block [3])"));
		}
		m_msg.append( msg_itr, flags_itr - msg_itr);

		rec.type = m_msg.c_str();
		std::size_t typesize = std::strlen( rec.type);
		rec.value = rec.type + typesize + 1;

		unsigned char flags = (unsigned char)*flags_itr;
		if (flags >= 0x2)
		{
			throw strus::runtime_error( _TXT( "got illegal message from  (corrupt message record [3])"));
		}
		chlen = utf8charlen( *df_itr);
		rec.increment = utf8decode( df_itr, chlen);
		if ((flags & 0x1) != 0)
		{
			rec.increment = -rec.increment;
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error statistics message viewer fetching next df change: %s"), *m_errorhnd, false);
}

