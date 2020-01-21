/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorIntersect.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>
#include <algorithm>

using namespace strus;

IteratorIntersect::IteratorIntersect( const std::vector<Reference< PostingIteratorInterface> >& args, ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args)
	,m_docnoAllMatchItr(args)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = m_argar.begin(), ae = m_argar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	m_featureid.push_back( 'I');
}

IteratorIntersectWithCardinality::IteratorIntersectWithCardinality(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		unsigned int cardinality_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_call_posno(0)
	,m_cardinality(cardinality_)
	,m_argar(args)
	,m_docnoMatchPrioQueue(args,cardinality_)
	,m_windowIsInitialized(false)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = m_argar.begin(), ae = m_argar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	if (cardinality_)
	{
		encodeInteger( m_featureid, cardinality_);
		m_featureid.push_back( 'C');
	}
	m_featureid.push_back( 'I');
}

IteratorIntersect::~IteratorIntersect()
{}

IteratorIntersectWithCardinality::~IteratorIntersectWithCardinality()
{}

Index IteratorIntersect::skipDocCandidate( const Index& docno_)
{
	return m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
}

Index IteratorIntersectWithCardinality::skipDocCandidate( const Index& docno_)
{
	Index next = m_docnoMatchPrioQueue.skipDocCandidate( docno_);
	m_windowIsInitialized &= (next && m_docno == next);
	return m_docno = next;
}

Index IteratorIntersect::skipDoc( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	while (m_docno)
	{
		if (skipPos(0)) return m_docno;
		m_docno = m_docnoAllMatchItr.skipDocCandidate( m_docno+1);
	}
	return m_docno;
}

Index IteratorIntersectWithCardinality::skipDoc( const Index& docno_)
{
	Index next = m_docnoMatchPrioQueue.skipDocCandidate( docno_);
	m_windowIsInitialized &= (next && m_docno == next);
	m_docno = next;
	while (m_docno)
	{
		if (skipPos(0)) return m_docno;
		m_docno = m_docnoMatchPrioQueue.skipDocCandidate( m_docno+1);
		m_windowIsInitialized = false;
		m_call_posno = 0;
	}
	return m_docno;
}

Index IteratorIntersect::skipPos( const Index& pos_)
{
	if (!m_docno) return m_posno=0;

	Index pos_iter = pos_;
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		pos_iter = (*ai)->skipPos( pos_iter);
		if (pos_iter == 0)
		{
			return m_posno=0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index pos_next = (*ai)->skipPos( pos_iter);
			if (pos_next != pos_iter)
			{
				if (pos_next == 0)
				{
					return m_posno=0;
				}
				pos_iter = pos_next;
				break;
			}
		}
		if (ai == ae)
		{
			return m_posno=pos_iter;
		}
	}
}

Index IteratorIntersectWithCardinality::skipPos( const Index& pos_)
{
	if (m_windowIsInitialized)
	{
		if (m_posno >= pos_ && pos_ >= m_call_posno)
		{
			return m_posno;
		}
	}
	else
	{
		DocnoMatchPrioQueue::CandidateList candiates = m_docnoMatchPrioQueue.getCandidateList();
		m_positionWindow.init( candiates.ar, candiates.arsize, 0, m_cardinality, strus::IndexRange()/*field*/, PositionWindow::MaxWin);
		m_windowIsInitialized = true;
		m_call_posno = 0;
	}
	return m_posno = m_positionWindow.skip( m_call_posno = pos_) ? m_positionWindow.pos() : 0;
}

Index IteratorIntersect::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = minDocumentFrequency( m_argar);
	}
	return m_documentFrequency;
}

Index IteratorIntersectWithCardinality::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = minDocumentFrequency( m_argar);
	}
	return m_documentFrequency;
}

Index IteratorIntersect::length() const
{
	if (!m_posno) return 0;

	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = m_argar.begin(), ae = m_argar.end();

	Index rt = 0;
	for (; ai != ae; ++ai)
	{
		rt = std::max( rt, (*ai)->length());
	}
	return rt;
}

Index IteratorIntersectWithCardinality::length() const
{
	if (!m_posno) return 0;

	std::vector<Reference< PostingIteratorInterface> >::const_iterator
		ai = m_argar.begin(), ae = m_argar.end();

	Index rt = 0;
	for (; ai != ae; ++ai)
	{
		rt = std::max( rt, (*ai)->length());
	}
	return rt;
}

PostingIteratorInterface* PostingJoinIntersect::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& itrs,
		int range,
		unsigned int cardinality) const
{
	if (range != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no range argument expected for '%s'"), "intersect");
		return 0;
	}
	if (itrs.size() == 0)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for '%s'"), "intersect");
		return 0;
	}
	try
	{
		if (cardinality && cardinality != itrs.size())
		{
			return new IteratorIntersectWithCardinality( itrs, cardinality, m_errorhnd);
		}
		else
		{
			return new IteratorIntersect( itrs, m_errorhnd);
		}
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "intersect", *m_errorhnd, 0);
}


StructView PostingJoinIntersect::view() const
{
	try
	{
		return Description( "intersect", _TXT("Get the set of postings (d,p) that are occurring in all argument sets"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "intersect", *m_errorhnd, StructView());
}



