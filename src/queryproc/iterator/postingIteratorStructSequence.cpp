/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorStructSequence.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorStructSequence::~IteratorStructSequence()
{}

IteratorStructSequence::IteratorStructSequence(
		int range_,
		const std::vector<Reference< PostingIteratorInterface> >& args,
		bool with_cut_,
		bool strict_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_docno_cut(0)
	,m_posno(0)
	,m_argar(with_cut_?(args.begin()+1):args.begin(),args.end())
	,m_docnoAllMatchItr(with_cut_?(args.begin()+1):args.begin(), args.end())
	,m_cut(with_cut_?*args.begin():Reference<PostingIteratorInterface>())
	,m_with_cut(with_cut_)
	,m_strict_incr(strict_?1:0)
	,m_range(range_)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	// Create feature identifier string:
	std::vector<Reference< PostingIteratorInterface> >::iterator
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
	m_featureid.push_back( 'S');
}

Index IteratorStructSequence::skipDocCandidate( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	if (m_docno)
	{
		m_docno_cut = m_cut.get()?m_cut->skipDocCandidate( m_docno):0;
	}
	return m_docno;
}

Index IteratorStructSequence::skipDoc( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	while (m_docno)
	{
		m_docno_cut = m_cut.get()?m_cut->skipDocCandidate( m_docno):0;
		if (skipPos(0))
		{
			return m_docno;
		}
		m_docno = m_docnoAllMatchItr.skipDocCandidate( m_docno+1);
	}
	m_docno_cut = 0;
	return m_docno;
}

Index IteratorStructSequence::positionCut( const Index& min_pos, const Index& max_pos)
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

Index IteratorStructSequence::skipPos( const Index& pos_)
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
		if (!min_pos) return m_posno=0;

		max_pos = min_pos;
		for (++ai; ai != ae; ++ai)
		{
			max_pos = (*ai)->skipPos( max_pos+m_strict_incr);
			if (!max_pos) return m_posno=0;

			if (max_pos - min_pos > dist)
			{
				pos_iter = max_pos - dist;
				break;
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

Index IteratorStructSequence::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = minDocumentFrequency( m_argar);
	}
	return m_documentFrequency;
}


PostingIteratorInterface* PostingJoinStructSequence::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no cardinality argument expected for '%s'"), "sequence_struct");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for 'sequence_struct'"));
		return 0;
	}
	try
	{
		return new IteratorStructSequence( range_, argitr, true/*with cut*/, true/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence_struct", *m_errorhnd, 0);
}

StructView PostingJoinStructSequence::view() const
{
	try
	{
		return Description( "sequence_struct", _TXT("Get the set of postings (d,p) that exist in the second argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| < |rj| for i<j and i>2. Additionally there must not exist a posting in the first argument set that is overlapped by the interval formed by the other argument postings"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence_struct", *m_errorhnd, StructView());
}


PostingIteratorInterface* PostingJoinSequence::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no cardinality argument expected for '%s'"), "sequence");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for '%s'"), "sequence");
		return 0;
	}
	try
	{
		return new IteratorStructSequence( range_, argitr, false/*without cut*/, true/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence", *m_errorhnd, 0);
}

StructView PostingJoinSequence::view() const
{
	try
	{
		return Description( "sequence", _TXT("Get the set of postings (d,p) that exist in the first argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| < |rj| for i<j"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence", *m_errorhnd, StructView());
}

PostingIteratorInterface* PostingJoinStructChain::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no cardinality argument expected for '%s'"), "chain_struct");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for 'chain_struct'"));
		return 0;
	}
	try
	{
		return new IteratorStructSequence( range_, argitr, true/*with cut*/, false/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "chain_struct", *m_errorhnd, 0);
}

StructView PostingJoinStructChain::view() const
{
	try
	{
		return Description( "chain_struct", _TXT("Get the set of postings (d,p) that exist in the second argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| <= |rj| for i<j and i>2. Additionally there must not exist a posting in the first argument set that is overlapped by the interval formed by the other argument postings"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "chain_struct", *m_errorhnd, StructView());
}


PostingIteratorInterface* PostingJoinChain::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no cardinality argument expected for '%s'"), "chain");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for '%s'"), "chain");
		return 0;
	}
	try
	{
		return new IteratorStructSequence( range_, argitr, false/*without cut*/, false/*strict*/, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "chain", *m_errorhnd, 0);
}

StructView PostingJoinChain::view() const
{
	try
	{
		return Description( "chain", _TXT("Get the set of postings (d,p) that exist in the first argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| <= |rj| for i<j"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "chain", *m_errorhnd, StructView());
}




