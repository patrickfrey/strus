/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorDifference.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>

using namespace strus;

IteratorDifference::IteratorDifference(
		const Reference<PostingIteratorInterface>& positive_,
		const Reference<PostingIteratorInterface>& negative_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_docno_neg(0)
	,m_positive( positive_)
	,m_negative( negative_)
	,m_errorhnd(errorhnd_)
{
	m_featureid.append( positive_->featureid());
	m_featureid.append( negative_->featureid());
	m_featureid.push_back( 'N');
}

IteratorDifference::~IteratorDifference()
{}

Index IteratorDifference::skipDocCandidate( const Index& docno_)
{
	m_docno = m_positive->skipDocCandidate( docno_);
	m_docno_neg = m_negative->skipDocCandidate( m_docno);
	return m_docno;
}

Index IteratorDifference::skipDoc( const Index& docno_)
{
	Index docno_iter = docno_;
	for (;;)
	{
		m_docno = m_positive->skipDoc( docno_iter);
		if (!m_docno)
		{
			break;
		}
		m_docno_neg = m_negative->skipDoc( m_docno);
		if (!skipPos(0))
		{
			docno_iter = m_docno + 1;
			continue;
		}
		break;
	}
	return m_docno;
}

Index IteratorDifference::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;
	if (!m_docno) return 0;

	for (;;)
	{
		Index pos_positive = m_positive->skipPos( pos_iter);
		if (!pos_positive) return 0;
		if (m_docno_neg == m_docno)
		{
			Index pos_negative = m_negative->skipPos( pos_positive);
			if (pos_negative == pos_positive)
			{
				pos_iter = pos_positive+1;
				continue;
			}
		}
		return pos_positive;
	}
}


PostingIteratorInterface* PostingJoinDifference::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& argitr,
		int range,
		unsigned int cardinality) const
{
	if (cardinality != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "diff");
		return 0;
	}
	if (range != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for '%s'"), "diff");
		return 0;
	}
	if (argitr.size() < 2)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "diff");
		return 0;
	}
	if (argitr.size() > 2)
	{
		m_errorhnd->report( _TXT( "too many arguments for '%s'"), "diff");
		return 0;
	}
	try
	{
		return new IteratorDifference( argitr[0], argitr[1], m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "diff", *m_errorhnd, 0);
}

PostingJoinOperatorInterface::Description PostingJoinDifference::getDescription() const
{
	try
	{
		return Description( "diff", _TXT("Get the set of postings (d,p) that are in the first argument set but not in the second"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "diff", *m_errorhnd, Description());
}

