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
#include "strus/base/utf8.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <arpa/inet.h>
#include "strus/base/stdint.h"

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

StatisticsBuilder::StatisticsBuilder( std::size_t maxblocksize_, ErrorBufferInterface* errorhnd)
	:m_lastkey()
	,m_hdr()
	,m_cnt(0)
	,m_content()
	,m_content_consumed(false)
	,m_nofDocumentsInsertedChange(0)
	,m_nofDocumentsInsertedChange_bk(0)
	,m_blocksize(0)
	,m_maxblocksize(maxblocksize_)
	,m_errorhnd(errorhnd)
{}

StatisticsBuilder::~StatisticsBuilder()
{}

void StatisticsBuilder::setNofDocumentsInsertedChange(
			int increment)
{
	try
	{
		int sum = m_nofDocumentsInsertedChange + increment;
		if (sum < std::numeric_limits<int32_t>::min() || sum > std::numeric_limits<int32_t>::max())
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseMaxNofItemsExceeded), _TXT( "number of documents inserted change value is out of range"));
		}
		else
		{
			m_nofDocumentsInsertedChange = sum;
		}
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder set number of documents inserted change: %s"), *m_errorhnd);
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
		std::string rec;
		std::size_t termtypesize = std::strlen(termtype);
		std::size_t termvaluesize = std::strlen(termvalue);
		rec.reserve( termtypesize + termvaluesize + 2);
		rec.append( termtype, termtypesize);
		rec.push_back( '\1');
		rec.append( termvalue, termvaluesize);
		if (rec.size() > (uint32_t)std::numeric_limits<int32_t>::max() -64)
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseValueOutOfRange), _TXT( "document frequency change term size is out of range"));
		}
		else
		{
			m_blocksize += termtypesize + termvaluesize;
			if (m_content.empty() || m_blocksize > m_maxblocksize)
			{
				newContent();
			}
			if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
			{
				m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseValueOutOfRange), _TXT( "df increment is out of range"));
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
			if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
			pldata.append( idxbuf, idxpos);

#ifdef STRUS_LOWLEVEL_DEBUG
			std::size_t itemidx = content.size();
#endif
			std::size_t ii = 0;
			for (; ii<m_lastkey.size() && ii<rec.size(); ++ii)
			{
				if (m_lastkey[ii] != rec[ ii]) break;
			}
			m_lastkey = rec;
			std::size_t commonsize = ii;
			std::size_t restsize = rec.size() - ii;
		
			idxpos = utf8encode( idxbuf, (int32_t)commonsize);
			if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
			content.append( idxbuf, idxpos);
		
			idxpos = utf8encode( idxbuf, (int32_t)(restsize) + pldata.size());
			if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
			content.append( idxbuf, idxpos);
			content.append( rec.c_str() + commonsize, restsize);
			char* ci = const_cast<char*>( std::strchr( content.c_str() + content.size() - restsize, '\1'));
			if (ci) *ci = '\0';
			content.append( pldata);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "BLOCK ";
			printRecord( std::cerr, content.c_str() + itemidx, content.size() - itemidx);
#endif
		}
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder add df change: %s"), *m_errorhnd);
}

void StatisticsBuilder::initHeader( StatisticsHeader* hdr)
{
	hdr->nofDocumentsInsertedChange = htonl( (uint32_t)(int32_t)m_nofDocumentsInsertedChange);
	m_nofDocumentsInsertedChange = 0;
	m_nofDocumentsInsertedChange_bk = 0;
}

void StatisticsBuilder::getBlock( const void*& blk, std::size_t& blksize)
{
	m_content_consumed = true;
	blk = m_content.front().c_str();
	blksize = m_content.front().size();
}

bool StatisticsBuilder::fetchMessage( const void*& blk, std::size_t& blksize)
{
	if (m_content.empty())
	{
		clear();
		return false;
	}
	try
	{
		if (m_content_consumed)
		{
			m_content.pop_front();
			m_content_consumed = false;
		}
		while (!m_content.empty())
		{
			const std::string& rt = m_content.front();
			StatisticsHeader* hdr = reinterpret_cast<StatisticsHeader*>( const_cast<char*>( rt.c_str()));
			initHeader( hdr);
			getBlock( blk, blksize);
			return true;
		}
		if (m_nofDocumentsInsertedChange)
		{
			initHeader( &m_hdr);
			blk = (void*)&m_hdr;
			blksize = sizeof( m_hdr);
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
	m_lastkey.clear();
	m_cnt = 0;
	m_content.clear();
	m_content_consumed = false;
	m_nofDocumentsInsertedChange = 0;
	m_nofDocumentsInsertedChange_bk = 0;
	m_blocksize = 0;
}

void StatisticsBuilder::newContent()
{
	StatisticsHeader hdr;
	m_content.push_back( std::string());
	std::string& blk = m_content.back();
	blk.reserve( m_maxblocksize);
	blk.append( (char*)&hdr, sizeof(hdr));
	m_blocksize = 0;
	m_lastkey.clear();
	m_cnt += 1;
}

void StatisticsBuilder::start()
{
	try
	{
		m_cnt = 0;
		m_nofDocumentsInsertedChange_bk = m_nofDocumentsInsertedChange;
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder start: %s"), *m_errorhnd);
}

void StatisticsBuilder::rollback()
{
	try
	{
		for (; m_cnt > 0; --m_cnt)
		{
			m_content.pop_back();
		}
		m_nofDocumentsInsertedChange = m_nofDocumentsInsertedChange_bk;
		m_nofDocumentsInsertedChange_bk = 0;
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder rollback: %s"), *m_errorhnd);
}


