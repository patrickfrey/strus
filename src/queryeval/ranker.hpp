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
#include <cstring>

namespace strus
{

/// \class Ranker
/// \brief Data structure to keep the N best ranked documents in sorted order
class Ranker
{
public:
	/// \brief Constructor
	Ranker( std::size_t maxNofRanks_)
		:m_maxNofRanks(maxNofRanks_),m_nofRanks(0)
	{
		if (maxNofRanks_ == 0) throw std::runtime_error( "illegal value for max number of ranks");
		std::memset( m_brute_index, 0, sizeof( m_brute_index));
	}
	~Ranker(){}

	void insert( const queryeval::WeightedDocument& doc)
	{
		if (m_maxNofRanks < MaxIndexSize)
		{
			bruteInsert( doc);
		}
		else
		{
			multisetInsert( doc);
		}
	}

	std::vector<queryeval::WeightedDocument> result( std::size_t firstRank) const
	{
		if (m_maxNofRanks < MaxIndexSize)
		{
			return bruteResult( firstRank);
		}
		else
		{
			return multisetResult( firstRank);
		}
	}

	std::size_t nofRanks() const
	{
		return m_nofRanks;
	}

private:
	void multisetInsert( const queryeval::WeightedDocument& doc)
	{
		m_rankset.insert( doc);
		if (m_maxNofRanks < ++m_nofRanks)
		{
			m_rankset.erase( m_rankset.begin());
		}
	}

	std::vector<queryeval::WeightedDocument> multisetResult( std::size_t firstRank) const
	{
		std::vector<queryeval::WeightedDocument> rt;
		RankSet::reverse_iterator ri=m_rankset.rbegin(),re=m_rankset.rend();
		std::size_t ridx = 0;
		for (; ri != re; ++ri,++ridx)
		{
			if (ridx >= firstRank) break;
		}
		for (; ri != re && ridx < m_nofRanks; ++ri,++ridx)
		{
			if (ridx > m_nofRanks) break;
			rt.push_back( *ri);
		}
		return rt;
	}

	void bruteInsert_at( std::size_t idx, const queryeval::WeightedDocument& doc)
	{
		std::size_t docix = m_nofRanks >= m_maxNofRanks ? m_brute_index[ idx] : m_nofRanks;
		std::memmove( m_brute_index+idx+1, m_brute_index+idx, MaxIndexSize-idx-1);
		m_brute_index[ idx] = docix;
		m_brute_ar[ docix] = doc;
	}

	void bruteInsert( const queryeval::WeightedDocument& doc)
	{
		if (!m_nofRanks)
		{
			m_brute_ar[ m_brute_index[ 0] = 0] = doc;
		}
		else
		{
			std::size_t first = 0;
			std::size_t last = m_nofRanks > m_maxNofRanks ? m_maxNofRanks:m_nofRanks;
			std::size_t mid = last-1;

			while (last - first > 2)
			{
				if (doc > m_brute_ar[ m_brute_index[ mid]])
				{
					last = mid;
					mid = (first + mid) >> 1;
				}
				else if (doc < m_brute_ar[ m_brute_index[ mid]])
				{
					first = mid+1;
					mid = (last + mid) >> 1;
				}
				else
				{
					bruteInsert_at( mid, doc);
					++m_nofRanks;
					return;
				}
			}
			for (; first < last; ++first)
			{
				if (doc > m_brute_ar[ m_brute_index[ first]])
				{
					bruteInsert_at( first, doc);
				}
			}
		}
		++m_nofRanks;
	}

	std::vector<queryeval::WeightedDocument> bruteResult( std::size_t firstRank) const
	{
		std::vector<queryeval::WeightedDocument> rt;
		std::size_t limit = m_nofRanks > m_maxNofRanks ? m_maxNofRanks:m_nofRanks;
		for (std::size_t ridx=firstRank; ridx<limit; ++ridx)
		{
			rt.push_back( m_brute_ar[ m_brute_index[ ridx]]);
		}
		return rt;
	}

private:
	typedef std::multiset<
			queryeval::WeightedDocument,
			std::less<queryeval::WeightedDocument>,
			LocalStructAllocator<queryeval::WeightedDocument> > RankSet;
	RankSet m_rankset;
	enum {MaxIndexSize=128};

	unsigned char m_brute_index[ MaxIndexSize];
	queryeval::WeightedDocument m_brute_ar[ MaxIndexSize];
	std::size_t m_maxNofRanks;
	std::size_t m_nofRanks;
};

}//namespace
#endif

