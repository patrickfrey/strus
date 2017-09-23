/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Iterator on the all document matches
#include "docnoAllMatchItr.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

DocnoAllMatchItr::DocnoAllMatchItr( const std::vector<PostingIteratorReference>& args_)
	:m_args(orderByDocumentFrequency( args_.begin(), args_.end()))
	,m_curdocno(0),m_curdocno_candidate(0)
{
	if (m_args.empty()) throw strus::runtime_error( "%s", _TXT("passed empty set of argument postings to docno all match iterator"));
}

DocnoAllMatchItr::DocnoAllMatchItr(
		std::vector<PostingIteratorReference>::const_iterator ai,
		const std::vector<PostingIteratorReference>::const_iterator& ae)
	:m_args( orderByDocumentFrequency( ai, ae))
	,m_curdocno(0),m_curdocno_candidate(0)
{}


Index DocnoAllMatchItr::skipDocCandidate( const Index& docno_)
{
	if (docno_ && m_curdocno_candidate == docno_) return m_curdocno_candidate;

	Index docno_iter = docno_;
	std::vector<PostingIteratorReference>::iterator ae = m_args.end();
	for (;;)
	{
		std::vector<PostingIteratorReference>::iterator ai = m_args.begin();

		docno_iter = (*ai)->skipDocCandidate( docno_iter);
		if (docno_iter == 0)
		{
			return m_curdocno_candidate=0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDocCandidate( docno_iter);
			if (docno_next == 0)
			{
				return m_curdocno_candidate=0;
			}
			if (docno_next != docno_iter)
			{
				docno_iter = docno_next;
				break;
			}
		}
		if (ai == ae)
		{
			return m_curdocno_candidate=docno_iter;
		}
	}
}

Index DocnoAllMatchItr::skipDoc( const Index& docno_)
{
	if (docno_ && m_curdocno == docno_) return m_curdocno;
	Index docno_iter = docno_;
	for (;;)
	{
		docno_iter = skipDocCandidate( docno_iter);
		if (!docno_iter)
		{
			return m_curdocno = 0;
		}
		std::vector<PostingIteratorReference>::iterator ai = m_args.begin(), ae = m_args.end();
		for (; ai != ae; ++ai)
		{
			Index a_docno = (*ai)->skipDoc( docno_iter);
			if (!a_docno)
			{
				++docno_iter;
				break;
			}
			if (docno_iter < a_docno)
			{
				docno_iter = a_docno;
				break;
			}
		}
		if (ai == ae)
		{
			return m_curdocno = docno_iter;
		}
	}
}

Index DocnoAllMatchItr::maxDocumentFrequency() const
{
	return m_args[0]->documentFrequency();
}

Index DocnoAllMatchItr::minDocumentFrequency() const
{
	return m_args.back()->documentFrequency();
}


