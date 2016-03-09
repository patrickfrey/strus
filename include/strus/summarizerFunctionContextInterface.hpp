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
/// \brief Interface for the context data of a processed summarizer
/// \file summarizerFunctionContextInterface.hpp
#ifndef _STRUS_SUMMARIZER_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_EXECUTION_CONTEXT_INTERFACE_HPP_INCLUDED
#include "strus/summarizationVariable.hpp"
#include "strus/summaryElement.hpp"
#include "strus/termStatistics.hpp"
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
	/// \param[in] name_ name of the summarization feature
	/// \param[in] postingIterator_ iterator on the matches of the summarization feature
	/// \param[in] variables_ list of variables attached to subexpressions of the matches (passed with postingIterator_)
	/// \param[in] weight_ weight of this summarization feature
	/// \param[in] stats_ global term statistics passed down with the query. If undefined, they can be defined by or estimated from the posting iterator data. 
	/// \remark Do call this method before calling getSummary the first time for not having incomplete results
	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* postingIterator_,
			const std::vector<SummarizationVariable>& variables_,
			float weight_,
			const TermStatistics& stats_)=0;

	/// \brief Get the summarization elements for one document
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<SummaryElement> getSummary( const Index& docno)=0;
};

}//namespace
#endif


