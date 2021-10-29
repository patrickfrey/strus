/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the interface for a viewer of a statistics message received (distributed index)
/// \file statisticsViewer.cpp
#include "statisticsViewer.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/utf8.hpp"
#include <arpa/inet.h>

using namespace strus;

StatisticsViewer::StatisticsViewer( const void* msgptr, std::size_t msgsize, ErrorBufferInterface* errorhnd_)
	:m_hdr(reinterpret_cast<const StatisticsHeader*>(msgptr))
	,m_msgptr((const char*)msgptr + sizeof(StatisticsHeader))
	,m_msgitr((const char*)msgptr + sizeof(StatisticsHeader))
	,m_msgend((const char*)msgptr + msgsize)
	,m_msgsize(msgsize - sizeof(StatisticsHeader))
	,m_strings()
	,m_errorhnd(errorhnd_)
{
	if (m_msgitr > m_msgend)
	{
		throw strus::runtime_error( "%s",  _TXT( "got illegal message (message size)"));
	}
}

StatisticsViewer::~StatisticsViewer(){}

int StatisticsViewer::nofDocumentsInsertedChange()
{
	int rt = (int32_t)ntohl( (uint32_t)m_hdr->nofDocumentsInsertedChange);
	return rt;
}

bool StatisticsViewer::nextDfChange( TermStatisticsChange& rec)
{
	try
	{
		if (m_msgitr == m_msgend) return false;
		std::size_t chlen = utf8charlen( *m_msgitr);
		if (m_msgitr + chlen + 1 >= m_msgend)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal statistics message (unexpected end of message [1])"));
		}
		int32_t commonbytes = utf8decode( m_msgitr, chlen);
		if (commonbytes < 0)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal statistics message (corrupt block [1])"));
		}
		m_msgitr += chlen;
		chlen = utf8charlen( *m_msgitr);
		if (m_msgitr + chlen + 1 >= m_msgend)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal message from  (unexpected end of message [2])"));
		}
		int32_t restlen = utf8decode( m_msgitr, chlen);
		m_msgitr += chlen;
		if (restlen < 0)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal message from  (corrupt block [2])"));
		}
		if (m_msgitr + restlen > m_msgend)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal message from  (unexpected end of message [3])"));
		}
		m_msg.resize( commonbytes);
		const char* msg_itr = m_msgitr;
		m_msgitr += restlen;
		char const* df_itr = utf8prev( m_msgitr);
		char const* flags_itr = utf8prev( df_itr);
		if (flags_itr < msg_itr)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal message from  (corrupt block [3])"));
		}
		m_msg.append( msg_itr, flags_itr - msg_itr);
		m_strings.push_back( m_msg);

		const char* typestr = m_strings.back();
		const char* typeend = std::strchr( typestr, ' ');
		std::size_t typesize = typeend - typestr;
		const char* value = typeend + 1;

		unsigned char flags = (unsigned char)*flags_itr;
		if (flags >= 0x2)
		{
			throw strus::runtime_error( "%s",  _TXT( "got illegal message from  (corrupt message record [3])"));
		}
		chlen = utf8charlen( *df_itr);
		int increment = utf8decode( df_itr, chlen);
		if ((flags & 0x1) != 0)
		{
			increment = -increment;
		}
		rec = TermStatisticsChange( std::string(typestr,typesize), value, increment);
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error statistics message viewer fetching next df change: %s"), *m_errorhnd, false);
}

