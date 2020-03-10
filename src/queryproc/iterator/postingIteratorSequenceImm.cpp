/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorSequenceImm.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorSequenceImm::~IteratorSequenceImm()
{}

IteratorSequenceImm::IteratorSequenceImm(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args)
	,m_docnoAllMatchItr(args)
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
	m_featureid.push_back( 'Y');
}

Index IteratorSequenceImm::skipDocCandidate( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	return m_docno;
}

Index IteratorSequenceImm::skipDoc( const Index& docno_)
{
	m_docno = m_docnoAllMatchItr.skipDocCandidate( docno_);
	while (m_docno)
	{
		if (skipPos(0)) return m_docno;
		m_docno = m_docnoAllMatchItr.skipDocCandidate( m_docno+1);
	}
	return 0;
}

Index IteratorSequenceImm::skipPos( const Index& pos_)
{
	if (m_argar.empty()) return m_posno=0;
	Index pos_iter = pos_;

	std::vector<Reference< PostingIteratorInterface> >::iterator
		ai = m_argar.begin(), ae = m_argar.end();
	for (;;ai = m_argar.begin())
	{
		Index start_pos = (*ai)->skipPos( pos_iter);
		if (!start_pos) return m_posno=0;

		Index next_pos = start_pos + (*ai)->length();
		for (++ai; ai != ae; ++ai)
		{
			Index pos = (*ai)->skipPos( next_pos);
			if (!pos) return m_posno=0;
			if (pos > next_pos)
			{
				pos_iter = start_pos + pos - next_pos;
				break;
			}
			next_pos = pos + (*ai)->length();
		}
		if (ai == ae)
		{
			return m_posno = start_pos;
		}
	}
}

GlobalCounter IteratorSequenceImm::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = minDocumentFrequency( m_argar);
	}
	return m_documentFrequency;
}

PostingIteratorInterface* PostingJoinSequenceImm::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (range_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no range argument expected for '%s'"), "sequence_imm");
		return 0;
	}
	if (cardinality_ != 0)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no cardinality argument expected for '%s'"), "sequence_imm");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "too few arguments for '%s'"), "sequence_imm");
		return 0;
	}
	try
	{
		return new IteratorSequenceImm( argitr, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence_imm", *m_errorhnd, 0);
}

StructView PostingJoinSequenceImm::view() const
{
	try
	{
		return Description( "sequence_imm", _TXT("Get the set of postings (d,p) that exist in the first argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri|+1 == |rj| for i<j"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "sequence_imm", *m_errorhnd, StructView());
}

