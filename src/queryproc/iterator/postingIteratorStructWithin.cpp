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
#include "postingIteratorStructWithin.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cstdlib>
/*[-]*/#include <iostream>

using namespace strus;

IteratorStructWithin::~IteratorStructWithin()
{}

IteratorStructWithin::IteratorStructWithin(
		int range_,
		const std::vector<Reference< PostingIteratorInterface> >& args,
		bool with_cut_,
		ErrorBufferInterface* errorhnd_)
	:m_docno(0)
	,m_docno_cut(0)
	,m_posno(0)
	,m_with_cut(with_cut_)
	,m_range(range_)
	,m_documentFrequency(-1)
	,m_errorhnd(errorhnd_)
{
	if (m_with_cut)
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
	if (m_with_cut)
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
	try
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
	CATCH_ERROR_MAP_RETURN( _TXT("error 'within' iterator getting subexpressions: %s"), *m_errorhnd, std::vector<const PostingIteratorInterface*>());
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
				if (!pos_next)
				{
					return m_posno=0;
				}

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
			if (m_with_cut)
			{
				if (!m_cut.get())
				{
					return m_posno=0;
				}
				else if (m_docno_cut == m_docno)
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
			else
			{
				return m_posno=(m_range>=0?min_pos:max_pos);
			}
		}
	}
}

GlobalCounter IteratorStructWithin::documentFrequency() const
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


PostingIteratorInterface* PostingJoinStructWithin::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "within_struct");
		return 0;
	}
	if (argitr.size() < 2)
	{
		m_errorhnd->report( _TXT( "too few arguments for 'within_struct'"));
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, true, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating 'within_struct' iterator: %s"), *m_errorhnd, 0);
}

PostingIteratorInterface* PostingJoinWithin::createResultIterator(
		const std::vector<Reference< PostingIteratorInterface> >& argitr,
		int range_,
		unsigned int cardinality_) const
{
	if (cardinality_ != 0)
	{
		m_errorhnd->report( _TXT( "no cardinality argument expected for '%s'"), "within");
		return 0;
	}
	if (argitr.size() < 1)
	{
		m_errorhnd->report( _TXT( "too few arguments for '%s'"), "within");
		return 0;
	}
	try
	{
		return new IteratorStructWithin( range_, argitr, false, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating 'within' iterator: %s"), *m_errorhnd, 0);
}

