/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for packing/unpacking messages with statistics used for query evaluation.
/// \file statisticsProcessorInterface.hpp
#ifndef _STRUS_STATISTICS_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_PROCESSOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsProcessorInterface.hpp"

namespace strus
{
///\brief Forward declaration
class FileLocatorInterface;
///\brief Forward declaration
class ErrorBufferInterface;

#define STATISTICS_FILE_PREFIX 		"stats_"
#define STATISTICS_FILE_EXTENSION 	".bin"

class StatisticsProcessor
	:public StatisticsProcessorInterface
{
public:
	/// \brief Constructor
	/// \param[in] filelocator_ interface to locate files to read or the working directory where to write files to
	/// \param[in] errorhnd_ reference to error buffer (ownership hold by caller)
	explicit StatisticsProcessor( const FileLocatorInterface* filelocator_, ErrorBufferInterface* errorhnd_);

	virtual ~StatisticsProcessor();

	virtual StatisticsViewerInterface* createViewer( const void* msgptr, std::size_t msgsize) const;

	virtual TimeStamp getUpperBoundTimeStamp( const std::string& path, const TimeStamp timestamp) const;

	virtual StatisticsMessage loadChangeMessage( const std::string& path, const TimeStamp& timestamp) const;

	virtual StatisticsBuilderInterface* createBuilder( const std::string& path) const;

	virtual StatisticsMapInterface* createMap( const std::string& config) const;

	virtual void releaseStatistics( const std::string& path, const TimeStamp& timestamp) const;

private:
	std::string getFullPath( const std::string& path) const;

private:
	ErrorBufferInterface* m_errorhnd;
	const FileLocatorInterface* m_filelocator;
};

}//namespace
#endif

