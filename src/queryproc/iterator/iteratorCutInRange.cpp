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
#include "queryproc/iterator/iteratorCutInRange.hpp"
#include <stdexcept>

using namespace strus;

IteratorCutInRange::IteratorCutInRange( const IteratorReference& first_, const IteratorReference& second_, const IteratorReference& cut_, const Index& range_, bool withFirstElemCut_, bool withLastElemCut_)
	:m_docno(0)
	,m_docno_cut(0)
	,m_first(first_)
	,m_second(second_)
	,m_cut(cut_)
	,m_range(range_)
	,m_withFirstElemCut(withFirstElemCut_)
	,m_withLastElemCut(withLastElemCut_)
{}

IteratorCutInRange::IteratorCutInRange( const IteratorCutInRange& o)
	:m_docno(o.m_docno)
	,m_docno_cut(o.m_docno_cut)
	,m_first(o.m_first->copy())
	,m_second(o.m_second->copy())
	,m_cut(o.m_cut->copy())
	,m_range(o.m_range)
	,m_withFirstElemCut(o.m_withFirstElemCut)
	,m_withLastElemCut(o.m_withLastElemCut)
{}

Index IteratorCutInRange::skipDoc( const Index& docno_)
{
	Index docno_iter = docno_;

	for (;;)
	{
		Index docno_first = m_first->skipDoc( docno_iter);
		Index docno_second = m_second->skipDoc( docno_iter);

		if (!docno_first || !docno_second)
		{
			return 0;
		}
		if (docno_first >= docno_second)
		{
			docno_iter = docno_first;
			if (docno_first == docno_second)
			{
				m_docno = docno_first;
				m_docno_cut = m_cut->skipDoc( docno_first);
				return m_docno;
			}
		}
		else
		{
			docno_iter = docno_second;
		}
	}
}

Index IteratorCutInRange::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;

	for (;;)
	{
		Index pos_first = m_first->skipPos( pos_iter);
		Index pos_second = m_second->skipPos( pos_iter);

		if (!pos_first || !pos_second)
		{
			return 0;
		}
		if (pos_first > pos_second)
		{
			pos_iter = pos_first;
		}
		else if (pos_first + m_range >= pos_second)
		{
			//... we have a match of the range
			pos_iter = pos_first;
			if (m_docno_cut == m_docno)
			{
				Index pos_cut = m_cut->skipPos( m_withFirstElemCut?pos_first:pos_first+1);
				if (pos_cut <= pos_second)
				{
					if (!pos_cut)
					{
						return pos_first;
					}
					if (!m_withLastElemCut && pos_cut == pos_second)
					{
						return pos_first;
					}
					++pos_iter;
					continue;
				}
			}
		}
		else
		{
			pos_iter = pos_second - m_range + 1;
		}
	}
}

float IteratorCutInRange::weight() const
{
	throw std::runtime_error("internal: weight not defined for cirange");
}


