/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for packing/unpacking messages with statistics used for query evaluation to other  storages.
/// \file statisticsProcessor.cpp
#include "statisticsProcessor.hpp"
#include "statisticsBuilder.hpp"
#include "statisticsViewer.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

StatisticsProcessor::StatisticsProcessor( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_){}

StatisticsProcessor::~StatisticsProcessor(){}

StatisticsViewerInterface* StatisticsProcessor::createViewer(
			const char* msgptr, std::size_t msgsize) const
{
	try
	{
		return new StatisticsViewer( msgptr, msgsize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create statistics message viewer: %s"), *m_errorhnd, 0);
}

StatisticsBuilderInterface* StatisticsProcessor::createBuilder( const BuilderOptions& options_) const
{
	try
	{
		return new StatisticsBuilder( (options_.set & BuilderOptions::InsertInLexicalOrder) != 0, options_.maxBlockSize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error create statistics message builder: %s"), *m_errorhnd, 0);
}


