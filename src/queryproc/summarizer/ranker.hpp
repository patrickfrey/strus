/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

