/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	QueryResult()
		:m_evaluationPass(0)
		,m_nofRanked(0)
		,m_nofVisited(0)
		,m_ranks(){}
	/// \brief Copy constructor
	QueryResult( const QueryResult& o)
		:m_evaluationPass(o.m_evaluationPass)
		,m_nofRanked(o.m_nofRanked)
		,m_nofVisited(o.m_nofVisited)
		,m_ranks(o.m_ranks){}
	/// \brief Constructor
	QueryResult(
			unsigned int evaluationPass_,
			unsigned int nofRanked_,
			unsigned int nofVisited_,
			const std::vector<ResultDocument>& ranks_)
		:m_evaluationPass(evaluationPass_)
		,m_nofRanked(nofRanked_)
		,m_nofVisited(nofVisited_)
		,m_ranks(ranks_){}

	/// \brief Get the last query evaluation pass used (level of selection features used)
	unsigned int evaluationPass() const				{return m_evaluationPass;}
	/// \brief Get the total number of matches that were ranked (after applying all query restrictions)
	unsigned int nofRanked() const					{return m_nofRanked;}
	/// \brief Get the total number of matches that were visited (after applying ACL restrictions, but before applying other restrictions)
	unsigned int nofVisited() const					{return m_nofVisited;}

	/// \brief Get the list of result elements
	const std::vector<ResultDocument>& ranks() const		{return m_ranks;}

private:
	unsigned int m_evaluationPass;			///< query evaluation passes used (level of selection features used)
	unsigned int m_nofRanked;			///< total number of matches for a query with applying restrictions (might be an estimate)
	unsigned int m_nofVisited;			///< total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	std::vector<ResultDocument> m_ranks;		///< list of result documents (part of the total result)
};

}//namespace
#endif

