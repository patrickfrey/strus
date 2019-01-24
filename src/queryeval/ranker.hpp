/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYEVAL_RANKER_HPP_INCLUDED
#define _STRUS_QUERYEVAL_RANKER_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/weightedDocument.hpp"
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
	/// \brief Constructor
	Ranker( std::size_t maxNofRanks_)
		:m_maxNofRanks(maxNofRanks_),m_nofRanks(0)
	{
		if (maxNofRanks_ == 0) throw strus::runtime_error( "%s",  _TXT( "illegal value for max number of ranks"));
		std::size_t nn = (m_maxNofRanks < MaxIndexSize)?m_maxNofRanks:MaxIndexSize;
		for (std::size_t ii=0; ii<nn; ++ii) m_brute_index[ii] = ii;
	}
	~Ranker(){}

	void insert( const WeightedDocument& doc)
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

	std::vector<WeightedDocument> result( std::size_t firstRank) const
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
	void multisetInsert( const WeightedDocument& doc)
	{
		m_rankset.insert( doc);
		if (m_maxNofRanks < ++m_nofRanks)
		{
			m_rankset.erase( m_rankset.begin());
		}
	}

	std::vector<WeightedDocument> multisetResult( std::size_t firstRank) const
	{
		std::vector<WeightedDocument> rt;
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

	void bruteInsert_at( std::size_t idx, const WeightedDocument& doc)
	{
		std::size_t docix = m_brute_index[ m_maxNofRanks-1];
		std::memmove( m_brute_index+idx+1, m_brute_index+idx, m_maxNofRanks-idx-1);
		m_brute_index[ idx] = docix;
		m_brute_ar[ docix] = doc;
	}

	void bruteInsert( const WeightedDocument& doc)
	{
		if (!m_nofRanks)
		{
			m_brute_ar[ m_brute_index[ 0] = 0] = doc;
		}
		else if (doc < lastRank())
		{
			if (m_nofRanks < m_maxNofRanks)
			{
				bruteInsert_at( m_nofRanks, doc);
			}
			else
			{
				return;
			}
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
					break;
				}
			}
			if (first == last && last < m_maxNofRanks)
			{
				bruteInsert_at( last, doc);
			}
		}
		++m_nofRanks;
	}

	std::vector<WeightedDocument> bruteResult( std::size_t firstRank) const
	{
		std::vector<WeightedDocument> rt;
		std::size_t limit = m_nofRanks > m_maxNofRanks ? m_maxNofRanks:m_nofRanks;
		for (std::size_t ridx=firstRank; ridx<limit; ++ridx)
		{
			rt.push_back( m_brute_ar[ m_brute_index[ ridx]]);
		}
		return rt;
	}

	const WeightedDocument& lastRank() const
	{
		static const WeightedDocument emptyRank( 0, std::numeric_limits<double>::max());
		if (m_nofRanks == 0) return emptyRank;
		std::size_t lastElemIdx = (m_nofRanks > m_maxNofRanks ? m_maxNofRanks:m_nofRanks) -1;
		return m_brute_ar[ m_brute_index[ lastElemIdx]];
	}

private:
	typedef std::multiset<
			WeightedDocument,
			std::less<WeightedDocument>,
			LocalStructAllocator<WeightedDocument> > RankSet;
	RankSet m_rankset;
	enum {MaxIndexSize=128};

	unsigned char m_brute_index[ MaxIndexSize];
	WeightedDocument m_brute_ar[ MaxIndexSize];
	std::size_t m_maxNofRanks;
	std::size_t m_nofRanks;
};

}//namespace
#endif

