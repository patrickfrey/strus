/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "iterator/iteratorStructWithin.hpp"
#include "iterator/iteratorHelpers.hpp"
#include <stdexcept>

using namespace strus;

IteratorStructWithin::IteratorStructWithin( const std::vector<IteratorReference>& group_, const IteratorReference& cut_, int range_)
	:m_docno(0)
	,m_docno_cut(0)
	,m_group(group_)
	,m_cut(cut_)
	,m_range(range_)
{}

IteratorStructWithin::IteratorStructWithin( const IteratorStructWithin& o)
	:m_docno(o.m_docno)
	,m_docno_cut(o.m_docno_cut)
	,m_cut(o.m_cut->copy())
	,m_range(o.m_range)
{
	std::vector<IteratorReference>::const_iterator pi = o.m_group.begin(), pe = o.m_group.end();
	for (; pi != pe; ++pi)
	{
		m_group.push_back( (*pi)->copy());
	}
}

Index IteratorStructWithin::skipDoc( const Index& docno_)
{
	m_docno = getFirstAllMatchDocno( m_group, docno_);
	if (m_docno)
	{
		if (m_cut.get() && m_cut->skipDoc( m_docno) == m_docno)
		{
			m_docno_cut = m_docno;
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
		std::vector<IteratorReference>::const_iterator pi = m_group.begin(), pe = m_group.end();
		if (pi == pe) return 0;

		min_pos = (*pi)->skipPos( pos_iter);
		if (!min_pos)
		{
			return 0;
		}
		max_pos = min_pos;

		for (++pi; pi != pe; ++pi)
		{
			Index pos_next = (*pi)->skipPos( pos_iter);
			if (!pos_next) return 0;

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
				pos_iter = min_pos +1;
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

float IteratorStructWithin::weight() const
{
	throw std::runtime_error("internal: weight not defined for cirange");
}


