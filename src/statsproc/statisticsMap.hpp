/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Standard implementation of the map of global statistics (in case of a distributed index)
/// \file statisticsMap.hpp
#ifndef _STRUS_STATISTICS_MAP_IMPL_HPP_INCLUDED
#define _STRUS_STATISTICS_MAP_IMPL_HPP_INCLUDED
#include "strus/statisticsMapInterface.hpp"
#include "strus/index.hpp"
#include "strus/base/lockfreeStringMap.hpp"
#include "strus/base/atomic.hpp"

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;
///\brief Forward declaration
class StatisticsProcessorInterface;


/// \brief Standard implementation of the map of global statistics (in case of a distributed index)
class StatisticsMap
	:public StatisticsMapInterface
{
public:
	StatisticsMap( int nofBlocks_, const StatisticsProcessorInterface* proc_, ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsMap();

	virtual void setNofDocumentsInsertedChange( int increment);

	virtual void addDfChange( const char* termtype, const char* termvalue, int increment);

	virtual bool processStatisticsMessage( const void* msgptr, std::size_t msgsize);
	
	virtual GlobalCounter nofDocuments();

	virtual GlobalCounter df( const std::string& termtype, const std::string& termvalue);

private:
	ErrorBufferInterface* m_errorhnd;
	const StatisticsProcessorInterface* m_proc;
	strus::LockfreeStringMap<GlobalCounter> m_map;
	AtomicCounter<GlobalCounter> m_nofDocuments;
	
	
};
}//namespace
#endif

