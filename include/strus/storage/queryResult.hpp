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
#include "strus/storage/index.hpp"
#include "strus/storage/resultDocument.hpp"
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <limits>

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
		,m_ranks()
		,m_summaryElements(){}
	/// \brief Copy constructor
	QueryResult( const QueryResult& o)
		:m_evaluationPass(o.m_evaluationPass)
		,m_nofRanked(o.m_nofRanked)
		,m_nofVisited(o.m_nofVisited)
		,m_ranks(o.m_ranks)
		,m_summaryElements(o.m_summaryElements){}

	QueryResult& operator=( const QueryResult& o)
		{m_evaluationPass=o.m_evaluationPass;m_nofRanked=o.m_nofRanked;m_nofVisited=o.m_nofVisited;m_ranks=o.m_ranks;m_summaryElements=o.m_summaryElements; return *this;}

	/// \brief Constructor
	/// \param[in] evaluationPass_ query evaluation passes used (level of selection features used)
	/// \param[in] nofRanked_ total number of matches for a query with applying restrictions (might be an estimate)
	/// \param[in] nofVisited_ total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	/// \param[in] ranks_ list of result documents (part of the total result)
	QueryResult(
			int evaluationPass_,
			int nofRanked_,
			int nofVisited_,
			const std::vector<ResultDocument>& ranks_,
			const std::vector<SummaryElement>& summaryElements_)
		:m_evaluationPass(evaluationPass_)
		,m_nofRanked(nofRanked_)
		,m_nofVisited(nofVisited_)
		,m_ranks(ranks_)
		,m_summaryElements(summaryElements_){}
#if __cplusplus >= 201103L
	QueryResult( QueryResult&& o)
		:m_evaluationPass(o.m_evaluationPass),m_nofRanked(o.m_nofRanked),m_nofVisited(o.m_nofVisited)
		,m_ranks(std::move(o.m_ranks)),m_summaryElements(std::move(o.m_summaryElements)){}
	QueryResult& operator=( QueryResult&& o)
		{m_evaluationPass=o.m_evaluationPass;m_nofRanked=o.m_nofRanked;m_nofVisited=o.m_nofVisited;m_ranks=std::move(o.m_ranks);m_summaryElements=std::move(o.m_summaryElements); return *this;}
	QueryResult(
			int evaluationPass_,
			int nofRanked_,
			int nofVisited_,
			std::vector<ResultDocument>&& ranks_,
			std::vector<SummaryElement>&& summaryElements_)
		:m_evaluationPass(evaluationPass_)
		,m_nofRanked(nofRanked_)
		,m_nofVisited(nofVisited_)
		,m_ranks(std::move(ranks_))
		,m_summaryElements(std::move(summaryElements_)){}
#endif

	/// \brief Merging of a list of ranklists to one ranklist with an optional maximum size limit
	/// \remark This function assumes that the input lists are ordered in descending order, higher weight first
	/// \param[in] results list of results to merge to one
	/// \param[in] minRank start index of the resulting ranklist, counted from 0
	/// \param[in] maxNofRanks size limit of the resulting ranklist
	/// \return the merged result
	static QueryResult merge( const std::vector<QueryResult>& results, int minRank, int maxNofRanks)
	{
		std::vector<ResultDocument> ranks_;
		std::vector<SummaryElement> summaryElements_;
		int evaluationPass_ = 0;
		int nofRanked_ = 0;
		int nofVisited_ = 0;
		if (maxNofRanks < 0) maxNofRanks = std::numeric_limits<int>::max();
		if (minRank == 0) minRank = 0; 
		int maxNofResults = minRank + maxNofRanks;

		std::map< std::string, std::map<std::string,double> > summaryElementMap;

		typedef std::vector<ResultDocument>::const_iterator ResultIter;
		typedef std::pair<ResultIter,ResultIter> ResultIterRange;
		std::vector<ResultIterRange> rankiters;
		std::vector<QueryResult>::const_iterator ri = results.begin(), re = results.end();
		for (; ri != re; ++ri)
		{
			std::vector<SummaryElement>::const_iterator
				si = ri->summaryElements().begin(),
				se = ri->summaryElements().end();
			for (; si != se; ++si)
			{
				summaryElementMap[ si->name()][ si->value()] += si->weight();
			}
			if (ri->ranks().begin() != ri->ranks().end())
			{
				rankiters.push_back( ResultIterRange( ri->ranks().begin(), ri->ranks().end()));
			}
			if (ri->evaluationPass() > evaluationPass_) evaluationPass_ = ri->evaluationPass();
			nofRanked_ += ri->nofRanked();
			nofVisited_ += ri->nofVisited();
		}
		int ni = 0;
		for (; !rankiters.empty() && ni < maxNofResults; ++ni)
		{
			std::vector<ResultIterRange>::iterator ii = rankiters.begin(), ie = rankiters.end(), im = rankiters.begin();
			for (++ii; ii!=ie; ++ii) if (ii->first->weight() > im->first->weight()) im = ii;

			if (minRank <= ni) ranks_.push_back( *im->first++);
			if (im->first == im->second) rankiters.erase( im);
		}
		std::map< std::string, std::map<std::string,double> >::const_iterator
			si = summaryElementMap.begin(), se = summaryElementMap.end();
		for (; si != se; ++si)
		{
			std::map<std::string,double>::const_iterator
				mi = si->second.begin(), me = si->second.end();
			for (; mi != me; ++mi)
			{
				summaryElements_.push_back( SummaryElement( si->first, mi->first, mi->second));
			}
		}
		return QueryResult( evaluationPass_, nofRanked_, nofVisited_, ranks_, summaryElements_);
	}

	/// \brief Get the last query evaluation pass used (level of selection features used)
	int evaluationPass() const					{return m_evaluationPass;}
	/// \brief Get the total number of matches that were ranked (after applying all query restrictions)
	int nofRanked() const						{return m_nofRanked;}
	/// \brief Get the total number of matches that were visited (after applying ACL restrictions, but before applying other restrictions)
	int nofVisited() const						{return m_nofVisited;}

	/// \brief Get the list of result elements with summary attributes
	/// \return the list of weighted result document references
	const std::vector<ResultDocument>& ranks() const		{return m_ranks;}
	/// \brief Get the list of summary elements of this result
	const std::vector<SummaryElement>& summaryElements() const	{return m_summaryElements;}


private:
	int m_evaluationPass;				///< query evaluation passes used (level of selection features used)
	int m_nofRanked;				///< total number of matches for a query with applying restrictions (might be an estimate)
	int m_nofVisited;				///< total number of matches for a query without applying restrictions but ACL restrictions (might be an estimate)
	std::vector<ResultDocument> m_ranks;		///< list of result documents (part of the total result)
	std::vector<SummaryElement> m_summaryElements;	///< global summary elements of this result
};

}//namespace
#endif

