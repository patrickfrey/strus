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
/// \brief Result of a query evaluation
/// \file "queryResult.hpp"
#ifndef _STRUS_QUERY_RESULT_HPP_INCLUDED
#define _STRUS_QUERY_RESULT_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/resultDocument.hpp"
#include <vector>
#include <string>
#include <utility>

namespace strus {

/// \class QueryResult
/// \brief Structure defining the result of a strus query
class QueryResult
{
public:
	/// \brief Default constructor
	QueryResult(){}
	/// \brief Copy constructor
	QueryResult( const QueryResult& o)
		:m_evaluationPass(o.m_evaluationPass)
		,m_nofDocumentsRanked(o.m_nofDocumentsRanked)
		,m_nofDocumentsVisited(o.m_nofDocumentsVisited)
		,m_ranks(o.m_ranks)
		,m_summaryElements(o.m_summaryElements){}
	/// \brief Constructor
	QueryResult(
			unsigned int evaluationPass_,
			unsigned int nofDocumentsRanked_,
			unsigned int nofDocumentsVisited_,
			const std::vector<ResultDocument>& ranks_,
			const std::vector<SummaryElement>& summaryElements_)
		:m_evaluationPass(evaluationPass_)
		,m_nofDocumentsRanked(nofDocumentsRanked_)
		,m_nofDocumentsVisited(nofDocumentsVisited_)
		,m_ranks(ranks_)
		,m_summaryElements(summaryElements_){}

	/// \brief Get the last query evaluation pass used (level of selection features used)
	unsigned int evaluationPass() const				{return m_evaluationPass;}
	/// \brief Get the total number of matches that were ranked (after applying all query restrictions)
	unsigned int nofDocumentsRanked() const				{return m_nofDocumentsRanked;}
	/// \brief Get the total number of matches that were visited (after applying ACL restrictions, but before applying other restrictions)
	unsigned int nofDocumentsVisited() const			{return m_nofDocumentsVisited;}

	/// \brief Get the list of result elements
	const std::vector<ResultDocument>& ranks() const		{return m_ranks;}

	/// \brief Get the list of overall (global) summary elements of this query
	const std::vector<SummaryElement>& summaryElements() const	{return m_summaryElements;}

private:
	unsigned int m_evaluationPass;			///< query evaluation passes used (level of selection features used)
	unsigned int m_nofDocumentsRanked;		///< total number of matches for a query with applying restrictions (might be an estimate)
	unsigned int m_nofDocumentsVisited;		///< total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	std::vector<ResultDocument> m_ranks;		///< list of result documents (part of the total result)
	std::vector<SummaryElement> m_summaryElements;	///< global summary elements of this query
};

}//namespace
#endif

