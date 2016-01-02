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
#include "postingIteratorIntersect.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorIntersect::IteratorIntersect( const std::vector<Reference< PostingIteratorInterface> >& args, ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_posno(0)
	,m_argar(args)
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

IteratorIntersect::~IteratorIntersect()
{}

Index IteratorIntersect::skipDocCandidate( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;

	Index docno_iter = docno_;
	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		docno_iter = (*ai)->skipDocCandidate( docno_iter);
		if (docno_iter == 0)
		{
			m_docno = 0;
			return 0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDocCandidate( docno_iter);
			if (docno_next != docno_iter)
			{
				if (docno_next == 0)
				{
					m_docno = 0;
					return 0;
				}
				docno_iter = docno_next;
				break;
			}
		}
		if (ai == ae)
		{
			return m_docno = docno_iter;
		}
	}
}

Index IteratorIntersect::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && docno_)
	{
		return m_docno;
	}
	Index docno_iter = docno_;
	for (;;)
	{
		if (!skipDocCandidate( docno_iter)) return 0;
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = m_argar.begin(), ae = m_argar.end();
		for (;ai != ae; ++ai)
		{
			if (m_docno != (*ai)->skipDoc( m_docno)) break;
		}
		if (ai != ae)
		{
			docno_iter = m_docno+1;
			continue;
		}
		if (!skipPos(0))
		{
			docno_iter = m_docno+1;
			continue;
		}
		break;
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
				if (pos_next == 0) return m_posno=0;
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

Index IteratorIntersect::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		std::vector<Reference< PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		m_documentFrequency = (*ai)->documentFrequency();
		for (++ai; ai != ae && m_documentFrequency < 0; ++ai)
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


PostingIteratorInterface* PostingJoinIntersect::createResultIterator(
		const std::vector<Reference<PostingIteratorInterface> >& itrs,
		int range,
		unsigned int cardinality) const
{
	if (cardinality != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "intersect");
		return 0;
	}
	if (range != 0)
	{
		m_errorhnd->report( _TXT( "no range argument expected for '%s'"), "intersect");
		return 0;
	}
	if (itrs.size() == 0)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "intersect");
		return 0;
	}
	try
	{
		return new IteratorIntersect( itrs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "intersect", *m_errorhnd, 0);
}


PostingJoinOperatorInterface::Description PostingJoinIntersect::getDescription() const
{
	try
	{
		return Description( _TXT("Get the set of postings (d,p) that are occurring in all argument sets"));
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' iterator: %s"), "intersect", *m_errorhnd, Description());
}

