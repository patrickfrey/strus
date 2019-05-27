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
#include "strus/statisticsIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/utf8.hpp"
#include "strus/base/hton.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <arpa/inet.h>
#include "strus/base/stdint.h"

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

namespace {
class StatisticsIteratorImpl
	:public StatisticsIteratorInterface
{
public:
	StatisticsIteratorImpl( std::vector<std::string>& ar_, const TimeStamp& timestamp_)
	{
		while (!ar_.empty())
		{
			m_ar.push_back( StatisticsMessage( ar_.back().c_str(), ar_.back().size(), timestamp_));
			ar_.pop_back();
		}
		m_iter = m_ar.begin();
	}
	StatisticsIteratorImpl( DatedFileList& filelist, const TimeStamp& timestamp_)
	{
		DatedFileList::Iterator iterator = filelist.getIterator( timestamp_);
		if (iterator.blob())
		{
			do
			{
				m_ar.push_back( StatisticsMessage( iterator.blob(), iterator.blobsize(), iterator.timestamp()));
			} while (iterator.next());
		}
		m_iter = m_ar.begin();
	}

	virtual StatisticsMessage getNext()
	{
		if (m_iter == m_ar.end()) return StatisticsMessage( NULL, 0, TimeStamp(0));
		return *m_iter++;
	}

private:
	std::vector<StatisticsMessage> m_ar;
	std::vector<StatisticsMessage>::const_iterator m_iter;
};
}


StatisticsBuilder::StatisticsBuilder( const std::string& path_, std::size_t maxchunksize_, ErrorBufferInterface* errorhnd_)
	:m_timestamp(0)
	,m_lastkey()
	,m_content()
	,m_nofDocumentsInsertedChange(0)
	,m_blocksize(0)
	,m_maxchunksize(maxchunksize_)
	,m_datedFileList( path_/*directory*/, "stats_"/*prefix*/, ".bin"/*extension*/)
	,m_errorhnd(errorhnd_)
{
	if (m_maxchunksize <= sizeof(StatisticsHeader)) throw std::runtime_error(_TXT("Maximum block size for statistics builder too small"));
	m_timestamp = m_datedFileList.currentTimestamp();
}

StatisticsBuilder::~StatisticsBuilder()
{}

void StatisticsBuilder::setNofDocumentsInsertedChange( int increment)
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

void StatisticsBuilder::moveNofDocumentsInsertedChange()
{
	if (m_content.empty()) newContent();
	StatisticsHeader* hdr = reinterpret_cast<StatisticsHeader*>( const_cast<char*>( m_content.back().c_str()));
	long sum = ByteOrder<int32_t>::ntoh( hdr->nofDocumentsInsertedChange);
	sum += m_nofDocumentsInsertedChange;
	if (sum < (long)std::numeric_limits<int32_t>::min() || sum > (long)std::numeric_limits<int32_t>::max())
	{
		throw std::runtime_error( _TXT( "number of documents inserted change value is out of range"));
	}
	hdr->nofDocumentsInsertedChange = ByteOrder<int32_t>::hton( sum);
	m_nofDocumentsInsertedChange = 0;
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
			m_errorhnd->report( ErrorCodeValueOutOfRange, _TXT( "document frequency change term size is out of range"));
		}
		else
		{
			m_blocksize += termtypesize + termvaluesize;
			if (m_content.empty())
			{
				newContent();
				moveNofDocumentsInsertedChange();
			}
			else if (m_blocksize > m_maxchunksize)
			{
				moveNofDocumentsInsertedChange();
				newContent();
			}
			else if (m_nofDocumentsInsertedChange)
			{
				moveNofDocumentsInsertedChange();
			}
			if (increment > std::numeric_limits<int32_t>::max() || increment < std::numeric_limits<int32_t>::min())
			{
				m_errorhnd->report( ErrorCodeValueOutOfRange, _TXT( "df increment is out of range"));
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

void StatisticsBuilder::clear()
{
	m_lastkey.clear();
	m_content.clear();
	m_nofDocumentsInsertedChange = 0;
	m_blocksize = 0;
}

void StatisticsBuilder::newContent()
{
	StatisticsHeader hdr;
	m_content.push_back( std::string());
	std::string& blk = m_content.back();
	blk.reserve( m_maxchunksize);
	blk.append( (char*)&hdr, sizeof(hdr));
	m_blocksize = 0;
	m_lastkey.clear();
}

bool StatisticsBuilder::commit()
{
	try
	{
		moveNofDocumentsInsertedChange();
		m_datedFileList.store( m_content, ".tmp");
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("unexpected exception in statistic builder commit: %s"), *m_errorhnd, false);
}

void StatisticsBuilder::rollback()
{
	try
	{
		m_content.clear();
		m_nofDocumentsInsertedChange = 0;
	}
	CATCH_ERROR_MAP( _TXT("error statistics message builder rollback: %s"), *m_errorhnd);
}

StatisticsIteratorInterface* StatisticsBuilder::createIteratorAndRollback()
{
	try
	{
		moveNofDocumentsInsertedChange();
		return new StatisticsIteratorImpl( m_content, m_timestamp);
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

StatisticsIteratorInterface* StatisticsBuilder::createIterator( const TimeStamp& timestamp_)
{
	try
	{
		return new StatisticsIteratorImpl( m_datedFileList, timestamp_);
		
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error release statistics: %s"), *m_errorhnd, NULL);
}



