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
#include "iterator/postingIteratorIntersect.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorIntersect::IteratorIntersect( std::size_t nofargs, PostingIteratorInterface** args)
	:m_docno(0)
	,m_posno(0)
	,m_argarsize(nofargs)
	,m_documentFrequency(-1)
{
	m_argar = (PostingIteratorInterface**)malloc( nofargs * sizeof(m_argar[0]));
	if (!m_argar) throw std::bad_alloc();
	try
	{
		std::size_t ii=0;
		for (; ii<nofargs; ++ii)
		{
			m_argar[ii] = args[ii];
			if (ii) m_featureid.push_back('=');
			m_featureid.append( args[ii]->featureid());
		}
		m_featureid.push_back( 'I');
	}
	catch (const std::bad_alloc&)
	{
		std::free( m_argar);
		throw std::bad_alloc();
	}
}

IteratorIntersect::~IteratorIntersect()
{
	std::size_t ii=0;
	for (; ii<m_argarsize; ++ii)
	{
		delete m_argar[ii];
	}
	std::free( m_argar);
}

std::vector<const PostingIteratorInterface*>
	IteratorIntersect::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argarsize);
		std::size_t ii=0;
		for (; ii<m_argarsize; ++ii)
		{
			rt.push_back( m_argar[ ii]);
		}
	}
	return rt;
}

Index IteratorIntersect::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;

	Index docno_iter = docno_;
	for (;;)
	{
		std::size_t ai = 0, ae = m_argarsize;
		if (ai == ae) return 0;

		docno_iter = m_argar[ai]->skipDoc( docno_iter);
		if (docno_iter == 0)
		{
			m_docno = 0;
			return 0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = m_argar[ai]->skipDoc( docno_iter);
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

Index IteratorIntersect::skipPos( const Index& pos_)
{
	if (!m_docno) return m_posno=0;

	Index pos_iter = pos_;
	for (;;)
	{
		std::size_t ai = 0, ae = m_argarsize;
		if (ai == ae) return 0;

		pos_iter = m_argar[ai]->skipPos( pos_iter);
		if (pos_iter == 0)
		{
			return m_posno=0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index pos_next = m_argar[ai]->skipPos( pos_iter);
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
		std::size_t ai = 0, ae = m_argarsize;
		if (ai == ae) return 0;

		m_documentFrequency = m_argar[ai]->documentFrequency();
		for (++ai; ai != ae && m_documentFrequency < 0; ++ai)
		{
			Index df = m_argar[ai]->documentFrequency();
			if (df < m_documentFrequency)
			{
				m_documentFrequency = df;
			}
		}
	}
	return m_documentFrequency;
}

