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
#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/statisticsIteratorInterface.hpp"
#include "strus/timeStamp.hpp"
#include "strus/errorCodes.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/utf8.hpp"
#include "strus/base/hton.hpp"
#include "strus/base/string_format.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <limits>
#include <arpa/inet.h>
#include "strus/base/stdint.h"

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

namespace {
class InPlaceStatisticsIterator
	:public StatisticsIteratorInterface
{
public:
	explicit InPlaceStatisticsIterator( std::vector<std::string>& ar_, const TimeStamp& timestamp = TimeStamp())
	{
		while (!ar_.empty())
		{
			m_ar.push_back( StatisticsMessage( ar_.back().c_str(), ar_.back().size(), TimeStamp()));
			ar_.pop_back();
		}
		std::reverse( m_ar.begin(), m_ar.end());
		m_itr = m_ar.begin();
	}

	virtual StatisticsMessage getNext()
	{
		if (m_itr == m_ar.end()) return StatisticsMessage( NULL, 0, TimeStamp(0));
		return *m_itr++;
	}

private:
	std::vector<StatisticsMessage> m_ar;
	std::vector<StatisticsMessage>::const_iterator m_itr;
};
}


StatisticsBuilder::StatisticsBuilder( const std::string& path_, std::size_t maxchunksize_, ErrorBufferInterface* errorhnd_)
	:m_timestamp(0)
	,m_dfChangeMap()
	,m_nofDocumentsInsertedChange(0)
	,m_maxchunksize(maxchunksize_)
	,m_datedFileList( path_/*directory*/, "stats_"/*prefix*/, ".bin"/*extension*/)
	,m_errorhnd(errorhnd_)
{
	if (m_maxchunksize <= sizeof(StatisticsHeader)) throw std::runtime_error(_TXT("Maximum block size for statistics builder too small"));
	m_timestamp = getCurrentTimeStamp();
}

StatisticsBuilder::~StatisticsBuilder()
{}

void StatisticsBuilder::addNofDocumentsInsertedChange( int increment)
{
	try
	{
		long sum = m_nofDocumentsInsertedChange + increment;
		if (sum < (long)std::numeric_limits<int32_t>::min() || sum > (long)std::numeric_limits<int32_t>::max())
		{
			m_errorhnd->report( ErrorCodeMaxNofItemsExceeded, _TXT( "number of documents inserted change value is out of range"));
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

static inline std::string getDfChangeKey( const char* termtype, const char* termvalue)
{
	std::string rt;
	rt.append( termtype);
	rt.push_back( '\0');
	rt.append( termvalue);
	return rt;
}

void StatisticsBuilder::addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment)
{
	try
	{
		if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
		{
			m_errorhnd->report( ErrorCodeValueOutOfRange, _TXT( "df increment is out of range"));
			return;
		}
		std::string key = getDfChangeKey( termtype, termvalue);
		if (key.size() > (uint32_t)std::numeric_limits<int32_t>::max() -64)
		{
			m_errorhnd->report( ErrorCodeValueOutOfRange, _TXT( "document frequency change term size is out of range"));
		}
		m_dfChangeMap[ key] += increment;
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder add df change: %s"), *m_errorhnd);
}

std::vector<std::string> StatisticsBuilder::getDfChangeMapBlocks()
{
	std::vector<std::string> rt;
	std::string emptystring;
	const std::string* lastkey = &emptystring;

	std::map<std::string,int>::const_iterator mi = m_dfChangeMap.begin(), me = m_dfChangeMap.end();
	for (; mi != me; lastkey=&mi->first,++mi)
	{
		const std::string& key = mi->first;
		int increment = mi->second;

		if (rt.empty() || rt.back().size() > m_maxchunksize)
		{
			rt.push_back( newContent());
			lastkey = &emptystring;
		}
		std::string& content = rt.back();

		std::string pldata;
		char idxbuf[ 32];
		unsigned char flags = 0x0;
		if (increment < 0)
		{
			flags |= 0x1;
			increment = -increment;
		}
		std::size_t idxpos = utf8encode( idxbuf, (int32_t)increment);
		if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);

		pldata.push_back( flags);
		pldata.append( idxbuf, idxpos);

#ifdef STRUS_LOWLEVEL_DEBUG
		std::size_t itemidx = content.size();
#endif
		std::size_t ii = 0;
		for (; ii<lastkey->size() && ii<key.size(); ++ii)
		{
			if ((*lastkey)[ii] != key[ ii]) break;
		}
		std::size_t commonsize = ii;
		std::size_t restsize = key.size() - ii;

		idxpos = utf8encode( idxbuf, (int32_t)commonsize);
		if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
		content.append( idxbuf, idxpos);			//[1] common size

		idxpos = utf8encode( idxbuf, (int32_t)(restsize) + pldata.size());
		if (!idxpos) throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
		content.append( idxbuf, idxpos);			//[2] restsize size + data (increment) size

		content.append( key.c_str() + commonsize, restsize);	//[3] rest key string
		content.append( pldata);				//[4] data (sign plus increment scalar value encoded as UTF-8)
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "BLOCK ";
		printRecord( std::cerr, content.c_str() + itemidx, content.size() - itemidx);
#endif
	}
	return rt;
}

void StatisticsBuilder::clear()
{
	m_dfChangeMap.clear();
	m_nofDocumentsInsertedChange = 0;
}

std::string StatisticsBuilder::newContent()
{
	StatisticsHeader hdr;
	std::string rt;
	rt.reserve( m_maxchunksize);
	hdr.nofDocumentsInsertedChange = ByteOrder<int32_t>::hton( m_nofDocumentsInsertedChange);
	m_nofDocumentsInsertedChange = 0;
	rt.append( (char*)&hdr, sizeof(hdr));
	return rt;
}

bool StatisticsBuilder::commit()
{
	try
	{
		std::vector<std::string> blocks = getDfChangeMapBlocks();
		m_datedFileList.store( blocks, ".tmp");
		clear();
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("unexpected exception in statistic builder commit: %s"), *m_errorhnd, false);
}

void StatisticsBuilder::rollback()
{
	clear();
}

StatisticsIteratorInterface* StatisticsBuilder::createIteratorAndRollback()
{
	try
	{
		std::vector<std::string> blocks = getDfChangeMapBlocks();
		TimeStamp currentTimestamp = getCurrentTimeStamp();

		StatisticsIteratorInterface* rt = new InPlaceStatisticsIterator( blocks, currentTimestamp);
		clear();
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error statistics message builder create iterator: %s"), *m_errorhnd, NULL);
}

void StatisticsBuilder::releaseStatistics( const TimeStamp& timestamp_)
{
	try
	{
		m_datedFileList.deleteFilesBefore( timestamp_);
	}
	CATCH_ERROR_MAP( _TXT("error release statistics: %s"), *m_errorhnd);
}




