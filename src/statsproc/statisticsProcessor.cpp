/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for packing/unpacking messages with statistics used for query evaluation to other  storages.
/// \file statisticsProcessor.cpp
#include "statisticsProcessor.hpp"
#include "statisticsBuilder.hpp"
#include "statisticsViewer.hpp"
#include "statisticsMap.hpp"
#include "datedFileList.hpp"
#include "strus/statisticsIteratorInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/constants.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/fileio.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

StatisticsProcessor::StatisticsProcessor( const FileLocatorInterface* filelocator_, ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_filelocator(filelocator_){}

StatisticsProcessor::~StatisticsProcessor(){}

std::string StatisticsProcessor::getFullPath( const std::string& path) const
{
	std::string workdir = m_filelocator->getWorkingDirectory();
	if (!workdir.empty())
	{
		if (strus::hasUpdirReference( path))
		{
			throw std::runtime_error( _TXT( "path for statistics processor must not contain up-directory references ('..') if workdir is specified"));
		}
		std::string rt = strus::joinFilePath( workdir, path);
		if (rt.empty()) throw std::bad_alloc();
		return rt;
	}
	return path;
}

StatisticsViewerInterface* StatisticsProcessor::createViewer(
			const void* msgptr, std::size_t msgsize) const
{
	try
	{
		return new StatisticsViewer( msgptr, msgsize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create statistics message viewer: %s"), *m_errorhnd, 0);
}

StatisticsBuilderInterface* StatisticsProcessor::createBuilder( const std::string& path) const
{
	try
	{
		int msgChunkSize = Constants::defaultStatisticsMsgChunkSize();
		return new StatisticsBuilder( getFullPath( path), msgChunkSize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create statistics message builder: %s"), *m_errorhnd, 0);
}


StatisticsMapInterface* StatisticsProcessor::createMap( const std::string& config) const
{
	try
	{
		std::string configstr = config;
		unsigned int nofBlocks = Constants::defaultStatisticsNofBlocks();
		if (!strus::extractUIntFromConfigString( nofBlocks, configstr, "blocks", m_errorhnd))
		{
			if (m_errorhnd->hasError()) throw std::runtime_error( m_errorhnd->fetchError());
		}
		return new StatisticsMap( nofBlocks, this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create statistics map: %s"), *m_errorhnd, 0);
}

namespace {
class StoredStatisticsIterator
	:public StatisticsIteratorInterface
{
public:
	StoredStatisticsIterator( DatedFileList& filelist, const TimeStamp& timestamp_)
		:m_itr(filelist.getIterator( timestamp_)){}

	virtual StatisticsMessage getNext()
	{
		if (m_itr.defined())
		{
			StatisticsMessage rt( m_itr.blob(), m_itr.blobsize(), m_itr.timestamp());
			(void)m_itr.next();
			return rt;
		}
		else
		{
			return StatisticsMessage();
		}
	}

private:
	DatedFileList::Iterator m_itr;
};
}
StatisticsIteratorInterface* StatisticsProcessor::createIterator( const std::string& path, const TimeStamp& timestamp) const
{
	try
	{
		DatedFileList filelist( getFullPath( path), Constants::defaultStatisticsFilePrefix(), Constants::defaultStatisticsFileExtension());
		return new StoredStatisticsIterator( filelist, timestamp);

	}
	CATCH_ERROR_MAP_RETURN( _TXT("error release statistics: %s"), *m_errorhnd, NULL);
}

std::vector<TimeStamp> StatisticsProcessor::getChangeTimeStamps( const std::string& path) const
{
	try
	{
		std::vector<TimeStamp> rt;
		DatedFileList filelist( getFullPath( path), Constants::defaultStatisticsFilePrefix(), Constants::defaultStatisticsFileExtension());
		DatedFileList::TimeStampIterator itr = filelist.getTimeStampIterator( TimeStamp(-1));
		TimeStamp tp = itr.timestamp();
		for (; tp >= 0; tp = itr.next())
		{
			rt.push_back( tp);
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting incremental statistic changes timestamps: %s"), *m_errorhnd, std::vector<TimeStamp>());
}

StatisticsMessage StatisticsProcessor::loadChangeMessage( const std::string& path, const TimeStamp& timestamp) const
{
	try
	{
		DatedFileList filelist( getFullPath( path), Constants::defaultStatisticsFilePrefix(), Constants::defaultStatisticsFileExtension());
		std::string blob = filelist.loadBlob( timestamp);
		return StatisticsMessage( blob, timestamp);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading a change message blob: %s"), *m_errorhnd, StatisticsMessage());
}



