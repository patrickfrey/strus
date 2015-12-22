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
#ifndef _STRUS_SUMMARIZER_RANKER_HPP_INCLUDED
#define _STRUS_SUMMARIZER_RANKER_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/localStructAllocator.hpp"
#include "private/internationalization.hpp"
#include <set>
#include <vector>
#include <cstring>
#include <limits>

namespace strus
{

/// \class Ranker
/// \brief Data structure to keep the N best ranked documents in sorted order
class Ranker
{
public:
	struct Element
	{
		Element()
			:weight(0.0),idx(0){}
		Element( double weight_, Index idx_)
			:weight(weight_),idx(idx_){}
		Element( const Element& o)
			:weight(o.weight),idx(o.idx){}

		bool operator<( const Element& o) const
		{
			if (weight < o.weight) return true;
			if (weight > o.weight + std::numeric_limits<double>::epsilon()) return false;
			return idx > o.idx;
		}

		double weight;
		Index idx;
	};

	/// \brief Constructor
	Ranker( std::size_t maxNofRanks_)
		:m_maxNofRanks(maxNofRanks_),m_nofRanks(0)
	{
		if (maxNofRanks_ == 0) throw strus::runtime_error( _TXT( "illegal value for max number of ranks"));
	}
	~Ranker(){}

	void insert( double weight, Index idx)
	{
		m_rankset.insert( Element( weight, idx));
		if (m_maxNofRanks <= m_nofRanks)
		{
			m_rankset.erase( m_rankset.begin());
		}
		else
		{
			++m_nofRanks;
		}
	}

	std::vector<Element> result() const
	{
		std::vector<Element> rt;
		RankSet::reverse_iterator ri=m_rankset.rbegin(),re=m_rankset.rend();
		for (; ri != re; ++ri)
		{
			rt.push_back( *ri);
		}
		return rt;
	}

private:
	typedef std::multiset<
			Element,
			std::less<Element>,
			LocalStructAllocator<Element> > RankSet;
	RankSet m_rankset;
	std::size_t m_maxNofRanks;
	std::size_t m_nofRanks;
};

}//namespace
#endif

