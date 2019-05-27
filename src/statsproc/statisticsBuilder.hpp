/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the interface for building messages to populate some statistics (distributed index)
/// \file statisticsBuilderInterface.hpp
#ifndef _STRUS_STATISTICS_BUILDER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_BUILDER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/base/stdint.h"
#include "statisticsHeader.hpp"
#include "datedFileList.hpp"
#include <string>
#include <vector>

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;
///\brief Forward declaration
class StatisticsIteratorInterface;

class StatisticsBuilder
	:public StatisticsBuilderInterface
{
public:
	StatisticsBuilder( const std::string& path_, std::size_t maxchunksize_, ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsBuilder();

	virtual void setNofDocumentsInsertedChange(
			int increment);

	virtual void addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment);

	virtual bool commit();

	virtual void rollback();

	virtual StatisticsIteratorInterface* createIteratorAndRollback();

	virtual void releaseStatistics( const TimeStamp& timestamp);

	virtual StatisticsIteratorInterface* createIterator( const TimeStamp& timestamp);

private:
	void moveNofDocumentsInsertedChange();
	void newContent();
	void clear();

private:
	TimeStamp m_timestamp;
	std::string m_lastkey;
	std::vector<std::string> m_content;
	int32_t m_nofDocumentsInsertedChange;
	std::size_t m_blocksize;
	std::size_t m_maxchunksize;
	DatedFileList m_datedFileList;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

