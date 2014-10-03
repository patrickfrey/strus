/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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

	virtual bool nextRank( Index& docno, int& state, double& weight)
	{
		switch (state)
		{
			case 0:
				docno = getNextAllMatchesDoc( docno);
				if (docno != 0)
				{
					weight = 0.0;
					std::vector<IteratorReference>::iterator ai = m_arg.begin(), ae = m_arg.end();
					for (; ai != ae; ++ai)
					{
						weight += WeightingFunction()( **ai);
					}
					return true;
				}
				docno = 0;
				state = 1;
				/* no break here! */
			case 1:
				docno = getNextMatchingDoc( docno, false);
				if (docno != 0)
				{
					weight = cur_weight();
					return true;
				}
				state = 2;
				/* no break here! */
			default:
				return false;
			break;
		}
	}

	virtual Index skipDoc( const Index& docno)
	{
		return getNextMatchingDoc( docno, true);
	}

	virtual double cur_weight()
	{
		double rt = 0.0;
		std::set<std::size_t>::const_iterator mi = m_matches.begin(), me = m_matches.end();
		for (; mi != me; ++mi)
		{
			rt += WeightingFunction()( *m_arg[ *mi]);
		}
		return rt;
	}

	virtual double min_weight()
	{
		return 0.0;
	}

	virtual double max_weight()
	{
		return std::numeric_limits<float>::max();
	}

private:
	Index getNextMatchingDoc( const Index& docno, bool withAllMatch)
	{
		std::size_t argidx = 0;
		m_matches.clear();
		std::vector<IteratorReference>::iterator ai = m_arg.begin(), ae = m_arg.end();
		if (ai == ae) return 0;

		Index min_docno = (*ai)->skipDoc( docno);
		m_matches.insert( 0);

		for (;;)
		{
			for (++ai; ai != ae; ++ai)
			{
				Index candidate_docno = (*ai)->skipDoc( docno);
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

	Index getNextAllMatchesDoc( const Index& docno)
	{
		if (m_arg.empty()) return 0;
		Index rt = docno;
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
	std::vector<IteratorReference> m_arg;
	std::set<std::size_t> m_matches;
};

}//namespace
#endif

