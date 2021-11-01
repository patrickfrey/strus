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
#include "datedFileList.hpp"
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

TimeStamp StatisticsProcessor::getUpperBoundTimeStamp( const std::string& path, const TimeStamp timestamp) const
{
	try
	{
		DatedFileList filelist( getFullPath( path), Constants::defaultStatisticsFilePrefix(), Constants::defaultStatisticsFileExtension());
		return filelist.getUpperBoundTimeStamp( timestamp);

	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting upperbound timestamp: %s"), *m_errorhnd, NULL);
}

StatisticsMessage StatisticsProcessor::loadChangeMessage( const std::string& path, TimeStamp timestamp) const
{
	try
	{
		DatedFileList filelist( getFullPath( path), Constants::defaultStatisticsFilePrefix(), Constants::defaultStatisticsFileExtension());
		std::string blob = filelist.loadBlob( timestamp);
		return StatisticsMessage( blob, timestamp);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error loading a change message blob: %s"), *m_errorhnd, StatisticsMessage());
}

void StatisticsProcessor::releaseStatistics( const std::string& path, TimeStamp timestamp) const
{
	try
	{
		DatedFileList fl( path/*directory*/, STATISTICS_FILE_PREFIX, STATISTICS_FILE_EXTENSION);
		fl.deleteFilesBefore( timestamp);
	}
	CATCH_ERROR_MAP( _TXT("error release statistics: %s"), *m_errorhnd);
}

