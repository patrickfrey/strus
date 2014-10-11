/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_ACCUMULATOR_OPERATOR_TEMPLATE_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_OPERATOR_TEMPLATE_HPP_INCLUDED
#include "strus/accumulatorInterface.hpp"
#include "strus/index.hpp"
#include <set>
#include <limits>

namespace strus
{

/// \class AccumulatorOperatorTemplate
/// \tparam WeightingFunction functor implementing
///		"float operator()( IteratorInterface&)"
/// \brief Base template class for accumulator operators on occurrence iterators summating the WeightingFunction result
template <class WeightingFunction>
class AccumulatorOperatorTemplate
	:public AccumulatorInterface
{
public:
	AccumulatorOperatorTemplate( const std::vector<IteratorReference>& arg_)
		:m_arg(arg_){}
	virtual ~AccumulatorOperatorTemplate(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weight_)
	{
		switch (state_)
		{
			case 0:
				docno_ = getNextAllMatchesDoc( docno_);
				if (docno_ != 0)
				{
					weight_ = 0.0;
					std::vector<IteratorReference>::iterator ai = m_arg.begin(), ae = m_arg.end();
					for (; ai != ae; ++ai)
					{
						weight_ += WeightingFunction()( **ai);
					}
					return true;
				}
				docno_ = 0;
				state_ = 1;
				/* no break here! */
			case 1:
				docno_ = getNextMatchingDoc( docno_, false);
				if (docno_ != 0)
				{
					weight_ = weight();
					return true;
				}
				state_ = 2;
				/* no break here! */
			default:
				return false;
			break;
		}
		return false;
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return getNextMatchingDoc( docno_, true);
	}

	virtual double weight()
	{
		double rt = 0.0;
		std::set<std::size_t>::const_iterator mi = m_matches.begin(), me = m_matches.end();
		for (; mi != me; ++mi)
		{
			rt += WeightingFunction()( *m_arg[ *mi]);
		}
		return rt;
	}

private:
	/// \brief Get the next (smallest document number) matching document
	/// \param[in] docno minimum document number to search for 
	/// \param[in] withAllMatch true => do not return matches of all documents (that were part of the set returned with getNextAllMatchesDoc(const Index&))
	Index getNextMatchingDoc( const Index& docno_, bool withAllMatch)
	{
		std::size_t argidx = 0;
		m_matches.clear();
		std::vector<IteratorReference>::iterator ai = m_arg.begin(), ae = m_arg.end();
		if (ai == ae) return 0;

		Index min_docno = (*ai)->skipDoc( docno_);
		m_matches.insert( 0);

		for (;;)
		{
			for (++ai; ai != ae; ++ai)
			{
				Index candidate_docno = (*ai)->skipDoc( docno_);
				if (candidate_docno == min_docno)
				{
					m_matches.insert( argidx);
				}
				else if (candidate_docno < min_docno)
				{
					min_docno = candidate_docno;
					m_matches.clear();
					m_matches.insert( argidx);
				}
			}
			if (!withAllMatch)
			{
				if (m_arg.size() == 1) return 0;
				if (m_matches.size() == m_arg.size())
				{
					ai = m_arg.begin();
					min_docno = (*ai)->skipDoc( min_docno+1);
					m_matches.insert( 0);
					continue;
				}
			}
			return min_docno;
		}
	}

	/// \brief Get the next (smallest document number) matching document appears in all input sets of occurrencies
	/// \param[in] docno minimum document number to search for 
	Index getNextAllMatchesDoc( const Index& docno_)
	{
		if (m_arg.empty()) return 0;
		Index rt = docno_;
		for (;;)
		{
			std::vector<IteratorReference>::iterator ai = m_arg.begin(), ae = m_arg.end();
			Index candidate_docno = (*ai)->skipDoc( rt);
			if (!candidate_docno) return 0;

			for (++ai; ai != ae; ++ai)
			{
				rt = (*ai)->skipDoc( candidate_docno);
				if (candidate_docno != rt)
				{
					rt = candidate_docno;
					break;
				}
			}
			if (ai == ae)
			{
				break;
			}
		}
		return rt;
	}

protected:
	std::vector<IteratorReference> m_arg;	///< input occurrencies to scan for results
	std::set<std::size_t> m_matches;	///< set of current matches
};

}//namespace
#endif

