/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerListMatches.hpp"
#include "private/ranker.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include "weightedValue.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("listmatch")

SummarizerFunctionContextListMatches::SummarizerFunctionContextListMatches(
		const StorageClientInterface* storage_,
		const MatchesBaseParameter& parameter_,
		ErrorBufferInterface* errorhnd_)
	:SummarizerFunctionContextMatchesBase( storage_, parameter_, THIS_METHOD_NAME, errorhnd_){}

SummarizerFunctionInstanceListMatches::SummarizerFunctionInstanceListMatches( ErrorBufferInterface* errorhnd_)
	:SummarizerFunctionInstanceMatchesBase( THIS_METHOD_NAME, errorhnd_){}

SummarizerFunctionListMatches::SummarizerFunctionListMatches( ErrorBufferInterface* errorhnd_)
	:SummarizerFunctionMatchesBase( THIS_METHOD_NAME, errorhnd_){}

std::vector<SummaryElement>
	SummarizerFunctionContextListMatches::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		start( doc);
		while (next() && (int)rt.size() < m_parameter.maxNofElements)
		{
			rt.push_back( SummaryElement( "", currentValue(), currentWeight(), currentIndex()));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface* storage,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( storage, m_parameter, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

SummarizerFunctionInstanceInterface* SummarizerFunctionListMatches::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceListMatches( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


