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
#include "iterator/postingIteratorStructWithin.hpp"
#include "iterator/postingIteratorHelpers.hpp"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstdlib>

using namespace strus;

IteratorStructWithin::~IteratorStructWithin()
{}

IteratorStructWithin::IteratorStructWithin(
		int range_,
		const std::vector<Reference< PostingIteratorInterface> >& args,
		bool with_cut)
	:m_docno(0)
	,m_docno_cut(0)
	,m_posno(0)
	,m_range(range_)
	,m_documentFrequency(-1)
{
	if (with_cut)
	{
		m_argar.insert( m_argar.end(), args.begin()+1, args.end());
		m_cut = args[0];
	}
	else
	{
		m_argar = args;
	}
	std::vector<Reference< PostingIteratorInterface> >::iterator
		ai = m_argar.begin(), ae = m_argar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) m_featureid.push_back('=');
		m_featureid.append( (*ai)->featureid());
	}
	if (with_cut)
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
	m_featureid.push_back( 'W');
}

std::vector<const PostingIteratorInterface*>
	IteratorStructWithin::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argar.size());
		std::vector<Reference< PostingIteratorInterface> >::const_iterator
			ai = m_argar.begin(), ae = m_argar.end();
		for (; ai != ae; ++ai)
		{
			rt.push_back( ai->get());
		}
	}
	else if (m_cut.get())
	{
		rt.push_back( m_cut.get());
	}
	return rt;
}

Index IteratorStructWithin::skipDoc( const Index& docno_)
{
	if (m_docno == docno_ && m_docno) return m_docno;

	m_docno = getFirstAllMatchDocno( m_argar, docno_);
	if (m_docno)
	{
		if (m_cut.get() && m_cut->skipDoc( m_docno) == m_docno)
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

Index IteratorStructWithin::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;
	Index min_pos = 0;
	Index max_pos = 0;
	Index rangenum = m_range>0?m_range:(-m_range);

	for (;;)
	{
		std::vector<Reference< PostingIteratorInterface> >::iterator
			ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		min_pos = (*ai)->skipPos( pos_iter);
		if (!min_pos)
		{
			return m_posno=0;
		}
		max_pos = min_pos;
		std::vector<Index> poset;	// .. linear search because sets are small
		poset.reserve( m_argar.size());
		poset.push_back( min_pos);

		for (++ai; ai != ae; ++ai)
		{
			Index pos_next = (*ai)->skipPos( pos_iter);
			for (;;)
			{
				if (!pos_next) return m_posno=0;

				if (std::find( poset.begin(), poset.end(), pos_next)==poset.end())
				{
					// ... only items at distinct positions are allowed
					break;
				}
				pos_next = (*ai)->skipPos( pos_next +1);
			}
			poset.push_back( pos_next);

			if (min_pos > pos_next)
			{
				min_pos = pos_next;
			}
			if (max_pos < pos_next)
			{
				max_pos = pos_next;
			}
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

Index IteratorStructWithin::documentFrequency() const
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


