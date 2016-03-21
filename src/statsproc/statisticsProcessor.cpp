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


