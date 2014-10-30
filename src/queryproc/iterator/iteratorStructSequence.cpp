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
#include "iterator/iteratorStructSequence.hpp"
#include "iterator/iteratorHelpers.hpp"
#include <stdexcept>

using namespace strus;

IteratorStructSequence::IteratorStructSequence( int range_, std::size_t nofargs, const IteratorInterface** args, const IteratorInterface* cut)
	:m_docno(0)
	,m_docno_cut(0)
	,m_range(range_)
{
	m_seq.reserve( nofargs);
	std::size_t ii=0;
	for (; ii<nofargs; ++ii)
	{
		if (args[ii])
		{
			m_seq.push_back( args[ii]->copy());
			m_featureid.append( args[ii]->featureid());
		}
	}
	if (cut)
	{
		m_cut.reset( cut->copy());
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

IteratorStructSequence::IteratorStructSequence( const IteratorStructSequence& o)
	:m_docno(o.m_docno)
	,m_docno_cut(o.m_docno_cut)
	,m_range(o.m_range)
	,m_featureid(o.m_featureid)
{
	m_seq.reserve( o.m_seq.size());
	if (o.m_cut.get())
	{
		m_cut.reset( o.m_cut->copy());
	}
	std::vector<IteratorReference>::const_iterator pi = o.m_seq.begin(), pe = o.m_seq.end();
	for (; pi != pe; ++pi)
	{
		m_seq.push_back( (*pi)->copy());
	}
}

std::vector<const IteratorInterface*> IteratorStructSequence::subExpressions( bool positive)
{
	std::vector<const IteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_seq.size());
		std::vector<IteratorReference>::const_iterator si = m_seq.begin(), se = m_seq.end();
		for (; si != se; ++si)
		{
			rt.push_back( si->get());
		}
	}
	else if (m_cut.get())
	{
		rt.push_back( m_cut.get());
	}
	return rt;
}

Index IteratorStructSequence::skipDoc( const Index& docno_)
{
	m_docno = getFirstAllMatchDocno( m_seq, docno_);
	if (m_docno)
	{
		if (m_cut.get() && m_cut->skipDoc( m_docno) == m_docno)
		{
			m_docno_cut = m_docno;
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
		std::vector<IteratorReference>::const_iterator pi = m_seq.begin(), pe = m_seq.end();
		if (pi == pe) return 0;

		min_pos = (*pi)->skipPos( pos_iter);
		if (!min_pos)
		{
			return 0;
		}
		max_pos = min_pos;

		for (++pi; pi != pe; ++pi)
		{
			max_pos = (*pi)->skipPos( max_pos+1);
			if (!max_pos) return 0;

			if (max_pos - min_pos > rangenum)
			{
				pos_iter = min_pos+1;
				break;
			}
		}
		if (pi == pe)
		{
			if (m_docno_cut == m_docno)
			{
				Index pos_cut = m_cut->skipPos( min_pos);
				if (pos_cut == 0 || pos_cut > max_pos)
				{
					return m_range>=0?min_pos:max_pos;
				}
				else
				{
					pos_iter = pos_cut + 1;
				}
			}
			else
			{
				return m_range>=0?min_pos:max_pos;
			}
		}
	}
}


