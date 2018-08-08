/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the interface for inspecting messages with statistics (distributed index)
/// \file statisticsViewerInterface.hpp
#ifndef _STRUS_STATISTICS_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsViewerInterface.hpp"
#include "statisticsHeader.hpp"
#include "private/stringMap.hpp"
#include <string>

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;

class StatisticsViewer
	:public StatisticsViewerInterface
{
public:
	StatisticsViewer( const void* msgptr, std::size_t msgsize, ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsViewer();

	virtual int nofDocumentsInsertedChange();

	virtual bool nextDfChange( TermStatisticsChange& rec);

private:
	const StatisticsHeader* m_hdr;
	const char* m_msgptr;
	char const* m_msgitr;
	char const* m_msgend;
	std::size_t m_msgsize;
	std::string m_msg;
	StringVector m_strings;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

