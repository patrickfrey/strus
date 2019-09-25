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
	/// \param[in] evaluationPass_ query evaluation passes used (level of selection features used)
	/// \param[in] nofRanked_ total number of matches for a query with applying restrictions (might be an estimate)
	/// \param[in] nofVisited_ total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	/// \param[in] ranks_ list of result documents (part of the total result)
	QueryResult(
			int evaluationPass_,
			int nofRanked_,
			int nofVisited_,
			const std::vector<ResultDocument>& ranks_)
		:m_evaluationPass(evaluationPass_)
		,m_nofRanked(nofRanked_)
		,m_nofVisited(nofVisited_)
		,m_ranks(ranks_){}

	/// \brief Merging of a list of ranklists to one ranklist with an optional maximum size limit
	/// \remark This function assumes that the input lists are ordered in descending order, higher weight first
	/// \param[in] results list of results to merge to one
	/// \param[in] maxNofResults optional size limit of the resulting ranklist
	/// \return the merged result
	static QueryResult merge( const std::vector<QueryResult>& results, int maxNofResults=-1)
	{
		std::vector<ResultDocument> ranks_;
		int evaluationPass_ = 0;
		int nofRanked_ = 0;
		int nofVisited_ = 0;

		typedef std::vector<ResultDocument>::const_iterator ResultIter;
		typedef std::pair<ResultIter,ResultIter> ResultIterRange;
		std::vector<ResultIterRange> rankiters;
		std::vector<QueryResult>::const_iterator ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			if (ri->ranks().begin() != ri->ranks().end())
			{
				rankiters.push_back( ResultIterRange( ri->ranks().begin(), ri->ranks().end()));
			}
			if (ri->evaluationPass() > evaluationPass_) evaluationPass_ = ri->evaluationPass();
			nofRanked_ += ri->nofRanked();
			nofVisited_ += ri->nofVisited();
		}
		while (!rankiters.empty() && (maxNofResults < 0 || (int)ranks_.size() < maxNofResults))
		{
			std::vector<ResultIterRange>::iterator ii = rankiters.begin(), ie = rankiters.end(), im = rankiters.begin();
			for (++ii; ii!=ie; ++ii) if (ii->first->weight() > im->first->weight()) im = ii;

			ranks_.push_back( *im->first++);
			if (im->first == im->second) rankiters.erase( im);
		}
		return QueryResult( evaluationPass_, nofRanked_, nofVisited_, ranks_);
	}

	/// \brief Get the last query evaluation pass used (level of selection features used)
	int evaluationPass() const				{return m_evaluationPass;}
	/// \brief Get the total number of matches that were ranked (after applying all query restrictions)
	int nofRanked() const					{return m_nofRanked;}
	/// \brief Get the total number of matches that were visited (after applying ACL restrictions, but before applying other restrictions)
	int nofVisited() const					{return m_nofVisited;}

	/// \brief Get the list of result elements with summary attributes
	/// \return the list of weighted result document references
	const std::vector<ResultDocument>& ranks() const	{return m_ranks;}

private:
	int m_evaluationPass;			///< query evaluation passes used (level of selection features used)
	int m_nofRanked;			///< total number of matches for a query with applying restrictions (might be an estimate)
	int m_nofVisited;			///< total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	std::vector<ResultDocument> m_ranks;	///< list of result documents (part of the total result)
};

}//namespace
#endif

