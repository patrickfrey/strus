/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorUnion.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstdlib>

using namespace strus;


IteratorUnion::IteratorUnion( const std::vector<Reference<PostingIteratorInterface> >& args_, ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args_)
#ifdef ALTERNATIVE_BITSET
	,m_selected(MaxNofElements)
#else
	,m_selected(0)
#endif
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	if (args_.size() > MaxNofElements)
	{
		throw strus::runtime_error( _TXT( "number of arguments of union out of range (> %u)"), (unsigned int)MaxNofElements);
	}
	std::vector<Reference<PostingIteratorInterface> >::const_iterator
		ai = args_.begin(), ae = args_.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	m_featureid.push_back( 'U');
}

IteratorUnion::~IteratorUnion()
{}

static inline Index selectSmallerNotNull( Index idx0, Index idx1)
{
	if (idx0 <= idx1)
	{
		if (idx0)
		{
			return idx0;
		}
		else
		{
			return idx1;
		}
	}
	else
	{
		if (idx1)
		{
			return idx1;
		}
		else
		{
			return idx0;
		}
	}
}

Index IteratorUnion::skipDocCandidate( const Index& docno_)
{
	if (m_docno == docno_ && docno_)
	{
		return m_docno;
	}
	m_docno = docno_;
	std::vector<Reference<PostingIteratorInterface> >::iterator
		ai = m_argar.begin(), ae = m_argar.end();
	Index base = docno_?docno_:1;
	Index minimum = 0;

	clearSelected();
	int aidx=0;
	for (; ai != ae; ++ai,++aidx)
	{
		minimum = (*ai)->skipDocCandidate( base);
		if (minimum) break;
	}
	if (!minimum)
	{
		m_docno = 0;
		return 0;
	}
	setSelected( aidx);

	for (aidx++,ai++; ai != ae; ++ai,++aidx)
	{
		Index next = (*ai)->skipDocCandidate( base);
		if (next && next <= minimum)
		{
			if (next < minimum)
			{
				clearSelected();
				minimum = next;
			}
			setSelected( aidx);
		}
	}
	m_docno = minimum;
	return m_docno;
}

Index IteratorUnion::skipDoc( const Index& docno_)
{
	Index docno_iter = docno_;
	for (;;)
	{
		docno_iter = skipDocCandidate( docno_iter);
		if (!docno_iter) return m_docno=0;

		selected_iterator si = selected_begin(), se = selected_end();
		for (unsigned int aidx=0; si != se; ++si,++aidx)
		{
			if (docno_iter == si->skipDoc( docno_iter)) break;
			unsetSelected( aidx); //... because we break, when we found one, we might not unset all non matching candidates
		}
#ifdef ALTERNATIVE_BITSET
		if (si == se && m_selected.empty())
#else
		if (si == se && !m_selected)
#endif
		{
			docno_iter += 1;
			continue;
		}
		break;
	}
	return m_docno = docno_iter;
}

Index IteratorUnion::skipPos( const Index& pos_)
{
	selected_iterator si = selected_begin(), se = selected_end();
	Index pos = 0;
	Index basepos = pos_?pos_:1;
	for (; si != se; ++si)
	{
		pos = si->skipPos( basepos);
		if (pos) break;
	}
	for (; si != se; ++si)
	{
		pos = selectSmallerNotNull( pos, si->skipPos( basepos));
	}
	return m_posno=pos;
}

Index IteratorUnion::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = maxDocumentFrequency( m_argar);
	}
	return m_documentFrequency;
}

PostingIteratorInterface* PostingJoinUnion::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& itrs,
		int range,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "union");
		return 0;
	}
	if (range != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for '%s'"), "union");
		return 0;
	}
	if (itrs.size() == 0)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "union");
		return 0;
	}
	try
	{
		return new IteratorUnion( itrs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "union", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinUnion::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings that are occurring in any argument set"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "union", *m_errorhnd, Description());
}

