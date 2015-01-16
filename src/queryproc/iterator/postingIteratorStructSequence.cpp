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
#include "iterator/postingIteratorStructSequence.hpp"
#include "iterator/postingIteratorHelpers.hpp"
#include <stdexcept>
#include <cstdlib>

using namespace strus;

IteratorStructSequence::~IteratorStructSequence()
{
	std::size_t ii=0;
	for (; ii<m_argarsize; ++ii)
	{
		delete m_argar[ii];
	}
	std::free( m_argar);
	if (m_cut) delete m_cut;
}

IteratorStructSequence::IteratorStructSequence(
		int range_,
		std::size_t nofargs,
		PostingIteratorInterface** args,
		PostingIteratorInterface* cut)
	:m_docno(0)
	,m_docno_cut(0)
	,m_posno(0)
	,m_argarsize(nofargs)
	,m_range(range_)
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
		m_cut = cut;
		if (cut)
		{
			if (nofargs) m_featureid.push_back('=');
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
	catch (const std::bad_alloc&)
	{
		std::free( m_argar);
		throw std::bad_alloc();
	}
}

std::vector<const PostingIteratorInterface*>
	IteratorStructSequence::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argarsize);
		std::size_t ii=0;
		for (; ii<m_argarsize; ++ii)
		{
			rt.push_back( m_argar[ii]);
		}
	}
	else if (m_cut)
	{
		rt.push_back( m_cut);
	}
	return rt;
}

Index IteratorStructSequence::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;

	m_docno = getFirstAllMatchDocno( m_argarsize, m_argar, docno_);
	if (m_docno)
	{
		if (m_cut && m_cut->skipDoc( m_docno) == m_docno)
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

Index IteratorStructSequence::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;
	Index min_pos = 0;
	Index max_pos = 0;
	Index rangenum = m_range>0?m_range:(-m_range);

	for (;;)
	{
		std::size_t ai = 0, ae = m_argarsize;
		if (ai == ae) return 0;

		min_pos = m_argar[ai]->skipPos( pos_iter);
		if (!min_pos)
		{
			return m_posno=0;
		}
		max_pos = min_pos;

		for (++ai; ai != ae; ++ai)
		{
			max_pos = m_argar[ai]->skipPos( max_pos+1);
			if (!max_pos) return m_posno=0;

			if (max_pos - min_pos > rangenum)
			{
				pos_iter = max_pos - rangenum;
				break;
			}
		}
		if (ai == ae)
		{
			if (m_docno_cut == m_docno)
			{
				Index pos_cut = m_cut->skipPos( min_pos);
				if (pos_cut == 0 || pos_cut > max_pos)
				{
					return m_posno=(m_range>=0?min_pos:max_pos);
				}
				else
				{
					pos_iter = pos_cut + 1;
				}
			}
			else
			{
				return m_posno=(m_range>=0?min_pos:max_pos);
			}
		}
	}
}

Index IteratorStructSequence::documentFrequency() const
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


