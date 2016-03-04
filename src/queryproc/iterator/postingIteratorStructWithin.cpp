/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "postingIteratorStructWithin.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace strus;

IteratorStructWithin::~IteratorStructWithin()
{}

struct IteratorDf
{
	Index argidx;
	Index df;

	IteratorDf( const Index& argidx_, const Index& df_)
		:argidx(argidx_),df(df_){}
	IteratorDf( const IteratorDf& o)
		:argidx(o.argidx),df(o.df){}

	bool operator<( const IteratorDf& o) const
	{
		if (df == o.df) return argidx < o.argidx;
		return df > o.df;
	}
};


IteratorStructWithin::IteratorStructWithin(
		int range_,
		const std::vector<Reference< PostingIteratorInterface> >& args,
		bool with_cut_,
		bool strict_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_docno_cut(0)
	,m_posno(0)
	,m_with_cut(with_cut_)
	,m_strict(strict_)
	,m_range(range_)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	// Some integrity checks:
	if (args.size() > MaxNofArguments)
	{
		throw strus::runtime_error(_TXT("too many arguments for '%s' join iterator"), "within");
	}
	if (args.size() < 1 + (m_with_cut?1:0))
	{
		throw strus::runtime_error(_TXT("too few arguments for '%s' join iterator"), "within");
	}
	// Sort the positive arguments by descending df and assign the cut element if defined:
	std::vector<IteratorDf> dfar;
	dfar.reserve( args.size());

	Index aidx = 0;
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = args.begin(), ae = args.end();
	if (m_with_cut)
	{
		m_cut = *ai;
		++aidx;
		++ai;
	}
	for (; ai != ae; ++ai,++aidx)
	{
		dfar.push_back( IteratorDf( aidx, (*ai)->documentFrequency()));
	}
	std::sort( dfar.begin(), dfar.end());

	m_argar.reserve( dfar.size());
	std::vector<IteratorDf>::const_iterator di = dfar.begin(), de = dfar.end();
	for (; di != de; ++di)
	{
		m_argar.push_back( args[ di->argidx]);
	}

	// Create feature identifier string:
	ai = m_argar.begin(), ae = m_argar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	if (m_with_cut)
	{
		m_featureid.push_back('=');
		m_featureid.append( m_cut->featureid());
		m_featureid.push_back( 'C');
	}
	if (m_range)
	{
		encodeInteger( m_featureid, m_range);
		m_featureid.push_back( 'R');
	}
	m_featureid.push_back( 'W');
}

Index IteratorStructWithin::skipDocCandidate( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;

	m_docno = getFirstAllMatchDocno( m_argar, docno_, true/*allow empty*/);
	if (m_docno)
	{
		if (m_cut.get() && m_cut->skipDocCandidate( m_docno) == m_docno)
		{
			m_docno_cut = m_docno;
		}
		else
		{
			m_docno_cut = 0;
		}
	}
	return m_docno;
}

Index IteratorStructWithin::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;
	Index docno_iter = docno_;

	for (;;)
	{
		m_docno = getFirstAllMatchDocno( m_argar, docno_iter, false/*allow empty*/);
		if (m_docno)
		{
			if (m_cut.get() && m_cut->skipDoc( m_docno) == m_docno)
			{
				m_docno_cut = m_docno;
			}
			else
			{
				m_docno_cut = 0;
			}
			if (!skipPos(0))
			{
				docno_iter = m_docno + 1;
				continue;
			}
		}
		break;
	}
	return m_docno;
}

struct WithinMatch
{
	Index pos;
	std::size_t argidx;

	WithinMatch(){}
	WithinMatch( const Index& pos_, std::size_t argidx_)
		:pos(pos_),argidx(argidx_){}
	WithinMatch( const WithinMatch& o)
		:pos(o.pos),argidx(o.argidx){}
};

struct WithinMatchArray
{
	WithinMatchArray()
		:size(0){}
	WithinMatchArray( const WithinMatchArray& o)
		:size(o.size)
	{
		std::memcpy( ar, o.ar, size * sizeof(*ar));
	}

	WithinMatch ar[ IteratorStructWithin::MaxNofArguments];
	std::size_t size;

	std::string tostring() const
	{
		std::ostringstream rt;
		for (std::size_t ii=0; ii<size; ++ii)
		{
			rt << " (" << ar[ii].pos << " [" << ar[ii].argidx << "])";
		}
		return rt.str();
	}

	void insert( const Index& pos, std::size_t argidx)
	{
		std::size_t wi=0;
		for (;wi<size && pos >= ar[wi].pos; ++wi){}
		if (wi < size)
		{
			std::memmove( ar+wi+1, ar+wi, (size-wi)*sizeof(WithinMatch));
		}
		ar[wi].pos = pos;
		ar[wi].argidx = argidx;
		++size;
	}

	bool hasConflicts() const
	{
		std::size_t wi=1;
		for (;wi<size; ++wi)
		{
			if (ar[wi].pos == ar[wi-1].pos) return true;
		}
		return false;
	}

	bool shiftPosition( std::size_t ai, std::vector<Reference< PostingIteratorInterface> >& arg)
	{
		PostingIteratorInterface* itr = arg[ ar[ ai].argidx].get();
		Index pos = itr->skipPos( ar[ ai].pos+1);
		if (!pos) return false;
		std::size_t argidx = ar[ ai].argidx;
		std::size_t wi=ai+1;
		for (;wi<size && pos >= ar[wi].pos; ++wi)
		{
			ar[ wi-1] = ar[ wi];
		}
		ar[ wi-1].pos = pos;
		ar[ wi-1].argidx = argidx;
		return true;
	}

	const Index& maxpos() const
	{
		return ar[ size-1].pos;
	}

	bool resolveConflicts( std::vector<Reference< PostingIteratorInterface> >& arg)
	{
		Index minpos = 0;
		std::vector<WithinMatchArray> conflictar;
		conflictar.push_back( *this);
		while (!conflictar.empty())
		{
			WithinMatchArray candidate1 = conflictar.back();
			conflictar.pop_back();

			std::size_t wi=1;
			for (;wi<candidate1.size; ++wi)
			{
				if (candidate1.ar[wi].pos == candidate1.ar[wi-1].pos) break;
			}
			if (wi < candidate1.size)
			{
				WithinMatchArray candidate2( candidate1);
				if (candidate2.shiftPosition( wi, arg))
				{
					if (!minpos || minpos > candidate2.maxpos())
					{
						if (candidate2.hasConflicts())
						{
							conflictar.push_back( candidate2);
						}
						else
						{
							minpos = candidate2.maxpos();
							*this = candidate2;
						}
					}
				}
				if (candidate1.shiftPosition( wi-1, arg))
				{
					if (!minpos || minpos > candidate1.maxpos())
					{
						if (candidate1.hasConflicts())
						{
							conflictar.push_back( candidate1);
						}
						else
						{
							minpos = candidate1.maxpos();
							*this = candidate1;
						}
					}
				}
			}
		}
		return minpos != 0;
	}
};

Index IteratorStructWithin::positionCut( const Index& min_pos, const Index& max_pos)
{
	Index rt = 0;
	if (!m_cut.get())
	{
		rt = 0;
	}
	else if (m_docno_cut == m_docno)
	{
		rt = m_cut->skipPos( min_pos);
		if (rt > max_pos)
		{
			rt = 0;
		}
	}
	return rt;
}

Index IteratorStructWithin::skipPos( const Index& pos_)
{
	if (m_argar.empty()) return m_posno=0;

	Index pos_iter = pos_;
	Index min_pos = 0;
	Index max_pos = 0;

	std::vector<Reference< PostingIteratorInterface> >::iterator
		ai = m_argar.begin(), ae = m_argar.end();
	Index dist;
	if (m_range >= 0)
	{
		dist = (Index)m_range;
	}
	else
	{
		dist = -(Index)m_range;
		pos_iter = (pos_iter > dist)?(pos_iter - dist):0;
	}
	for (;;ai = m_argar.begin())
	{
		min_pos = (*ai)->skipPos( pos_iter);
		if (!min_pos) return m_posno = 0;

		max_pos = min_pos;
		for (++ai; ai != ae; ++ai)
		{
			Index pos_next = (*ai)->skipPos( pos_iter);
			if (!pos_next) return m_posno = 0;

			if (min_pos > pos_next)
			{
				min_pos = pos_next;
			}
			if (max_pos < pos_next)
			{
				max_pos = pos_next;
			}
			if (max_pos - min_pos > dist)
			{
				pos_iter = max_pos - dist;
				break;
			}
			if (m_range < 0 && max_pos < pos_)
			{
				pos_iter = min_pos + 1;
				break;
			}
		}
		if (ai == ae)
		{
			if (m_strict)
			{
				WithinMatchArray war;
				std::size_t aidx = 0;
				for (ai=m_argar.begin(); ai != ae; ++ai,++aidx)
				{
					war.insert( (*ai)->posno(), aidx);
				}
				if (war.hasConflicts())
				{
					if (!war.resolveConflicts( m_argar))
					{
						pos_iter = min_pos + 1;
						if (pos_iter + dist < max_pos)
						{
							pos_iter = max_pos - dist;
						}
						continue;
					}
					else
					{
						max_pos = war.maxpos();
						if (max_pos - min_pos > dist)
						{
							pos_iter = max_pos - dist;
							continue;
						}
						for (std::size_t wi=0; wi<war.size; ++wi)
						{
							m_argar[ war.ar[ wi].argidx]->skipPos( war.ar[ wi].pos);
							// ... we have to be sure that all iterators are on the position of the match (because of variables for information extraction)
						}
					}
				}
			}
			if (m_with_cut)
			{
				Index pos_cut = positionCut( min_pos, max_pos);
				if (pos_cut)
				{
					pos_iter = pos_cut + 1;
				}
				else if (m_range >= 0)
				{
					return m_posno = min_pos;
				}
				else
				{
					return m_posno = max_pos;
				}
			}
			else if (m_range >= 0)
			{
				return m_posno = min_pos;
			}
			else
			{
				return m_posno = max_pos;
			}
		}
	}
}

Index IteratorStructWithin::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		std::vector<Reference< PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		m_documentFrequency = (*ai)->documentFrequency();
		for (++ai; ai != ae; ++ai)
		{
			Index df = (*ai)->documentFrequency();
			if (df < m_documentFrequency)
			{
				m_documentFrequency = df;
			}
		}
	}
	return m_documentFrequency;
}


PostingIteratorInterface* PostingJoinStructWithin::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "within_struct");
		return 0;
	}
	if (argitr.size() < 2)
	{
		m_errorhnd->report( _TXT( "too few arguments for 'within_struct'"));
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, true/*with cut*/, true/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "within_struct", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinStructWithin::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p) that exist in any argument set and distinct (d,p+r) exist in all other argument sets with |r| <= |range|. Additionally there must not exist a posting in the first argument set that is overlapped by the interval formed by the other argument postings."));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "within_struct", *m_errorhnd, Description());
}

PostingIteratorInterface* PostingJoinWithin::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "within");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "within");
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, false/*without cut*/, true/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "within", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinWithin::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p) that exist in any argument set and distinct (d,p+r) exist in all other argument sets with |r| <= |range|"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "within", *m_errorhnd, Description());
}

PostingIteratorInterface* PostingJoinStructInRange::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "inrange_struct");
		return 0;
	}
	if (argitr.size() < 2)
	{
		m_errorhnd->report( _TXT( "too few arguments for 'inrange_struct'"));
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, true/*with cut*/, false/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "inrange_struct", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinStructInRange::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p) that exist in any argument set and (d,p+r) exist in all other argument sets with |r| <= |range|. Additionally there must not exist a posting in the first argument set that is overlapped by the interval formed by the other argument postings."));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "inrange_struct", *m_errorhnd, Description());
}

PostingIteratorInterface* PostingJoinInRange::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "inrange");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "inrange");
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, false/*without cut*/, false/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "inrange", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinInRange::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p) that exist in any argument set and (d,p+r) exist in all other argument sets with |r| <= |range|"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "inrange", *m_errorhnd, Description());
}


