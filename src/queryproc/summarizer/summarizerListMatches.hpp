/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#define _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#include "summarizerMatchesBase.hpp"

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class SummarizerFunctionContextListMatches
	:public SummarizerFunctionContextMatchesBase
{
public:
	SummarizerFunctionContextListMatches(
			const StorageClientInterface* storage_,
			const MatchesBaseParameter& parameter_,
			ErrorBufferInterface* errorhnd_);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);
};


class SummarizerFunctionInstanceListMatches
	:public SummarizerFunctionInstanceMatchesBase
{
public:
	explicit SummarizerFunctionInstanceListMatches( ErrorBufferInterface* errorhnd_);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const;
	virtual bool doPopulate() const
	{
		return false;
	}
};


class SummarizerFunctionListMatches
	:public SummarizerFunctionMatchesBase
{
public:
	explicit SummarizerFunctionListMatches( ErrorBufferInterface* errorhnd_);

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;
};

}//namespace
#endif


