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
#ifndef _STRUS_QUERYEVAL_RANKER_HPP_INCLUDED
#define _STRUS_QUERYEVAL_RANKER_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/queryeval/weightedDocument.hpp"
#include "localStructAllocator.hpp"
#include <set>

namespace strus
{

/// \class Ranker
/// \brief Data structure to keep the N best ranked documents in sorted order
class Ranker
{
public:
	/// \brief Constructor
	Ranker( std::size_t maxNofRanks_)
		:m_maxNofRanks(maxNofRanks_),m_nofRanks(0){}
	~Ranker(){}

	void insert( const queryeval::WeightedDocument& doc)
	{
		m_rankset.insert( doc);
		if (m_maxNofRanks < ++m_nofRanks)
		{
			m_rankset.erase( m_rankset.begin());
		}
	}

	std::vector<queryeval::WeightedDocument> result( std::size_t firstRank) const
	{
		std::vector<queryeval::WeightedDocument> rt;
		RankSet::reverse_iterator ri=m_rankset.rbegin(),re=m_rankset.rend();
		for (std::size_t ridx=0; ri != re; ++ri,++ridx)
		{
			if (ridx >= firstRank) break;
		}
		for (; ri != re; ++ri)
		{
			rt.push_back( *ri);
		}
		return rt;
	}

	std::size_t nofRanks() const
	{
		return m_nofRanks;
	}

private:
	typedef std::multiset<
			queryeval::WeightedDocument,
			std::less<queryeval::WeightedDocument>,
			LocalStructAllocator<queryeval::WeightedDocument> > RankSet;
	RankSet m_rankset;
	std::size_t m_maxNofRanks;
	std::size_t m_nofRanks;
};

}//namespace
#endif

