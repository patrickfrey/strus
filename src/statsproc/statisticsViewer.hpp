/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Implementation of the interface for inspecting messages with statistics (distributed index)
/// \file statisticsViewerInterface.hpp
#ifndef _STRUS_STATISTICS_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_VIEWER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsViewerInterface.hpp"
#include "statisticsHeader.hpp"
#include <stdint.h>
#include <string>

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;

class StatisticsViewer
	:public StatisticsViewerInterface
{
public:
	StatisticsViewer( const char* msgptr, std::size_t msgsize, ErrorBufferInterface* errorhnd_);
	virtual ~StatisticsViewer();

	virtual int nofDocumentsInsertedChange();

	virtual bool nextDfChange( DocumentFrequencyChange& rec);

private:
	const StatisticsHeader* m_hdr;
	const char* m_msgptr;
	char const* m_msgitr;
	char const* m_msgend;
	std::size_t m_msgsize;
	std::string m_msg;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

