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
	if (m_args.empty()) throw strus::runtime_error(_TXT("passed empty set of argument postings to docno all match iterator"));
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
			if (docno_iter != (*ai)->skipDoc( docno_iter)) break;
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


