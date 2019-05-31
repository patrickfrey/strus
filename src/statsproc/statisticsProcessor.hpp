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
class ErrorBufferInterface;

class StatisticsProcessor
	:public StatisticsProcessorInterface
{
public:
	StatisticsProcessor( int nofBlocks_, int msgChunkSize_, ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsProcessor();

	virtual StatisticsViewerInterface* createViewer( const void* msgptr, std::size_t msgsize) const;

	virtual StatisticsIteratorInterface* createIterator( const std::string& path, const TimeStamp& timestamp) const;

	virtual std::vector<TimeStamp> getChangeTimeStamps( const std::string& path) const;

	virtual StatisticsMessage loadChangeMessage( const std::string& path, const TimeStamp& timestamp) const;

	virtual StatisticsBuilderInterface* createBuilder( const std::string& path) const;

	virtual StatisticsMapInterface* createMap() const;
	
private:
	ErrorBufferInterface* m_errorhnd;
	int m_nofBlocks;
	int m_msgChunkSize;
};

}//namespace
#endif

