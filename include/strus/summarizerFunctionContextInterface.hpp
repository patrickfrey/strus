/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for the context data of a processed summarizer
/// \file summarizerFunctionContextInterface.hpp
#ifndef _STRUS_SUMMARIZER_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/storage/summarizationVariable.hpp"
#include "strus/storage/summaryElement.hpp"
#include "strus/storage/termStatistics.hpp"
#include "strus/storage/weightedDocument.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class PostingIteratorInterface;

/// \class SummarizerFunctionContextInterface
/// \brief Interface for the summarization execution context
class SummarizerFunctionContextInterface
{
public:
	/// \brief Destructor
	virtual ~SummarizerFunctionContextInterface(){}

	/// \brief Add a sumarization feature that is subject of summarization to the execution context
	/// \param[in] role_ name of the role of the summarization feature
	/// \param[in] postingIterator_ iterator on the matches of the summarization feature (ownership remains at caller)
	/// \param[in] variables_ list of variables attached to subexpressions of the matches (passed with postingIterator_)
	/// \param[in] weight_ weight of this summarization feature
	/// \param[in] stats_ global term statistics passed down with the query. If undefined, they can be defined by or estimated from the posting iterator data. 
	/// \remark Do call this method before calling getSummary the first time for not having incomplete results
	virtual void addSummarizationFeature(
			const std::string& role_,
			PostingIteratorInterface* postingIterator_,
			const std::vector<SummarizationVariable>& variables_,
			double weight_,
			const TermStatistics& stats_)=0;

	/// \brief Set the value of a query variable
	/// \param[in] name name of the variable
	/// \param[in] value value of the variable
	virtual void setVariableValue( const std::string& name, double value)=0;

	/// \brief Get the summarization elements for one document
	/// \param[in] doc weighted document or passage to get the summary element from
	/// \return the summarization elements
	virtual std::vector<SummaryElement> getSummary( const WeightedDocument& doc)=0;
};

}//namespace
#endif


