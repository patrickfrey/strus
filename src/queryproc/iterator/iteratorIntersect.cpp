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
#include "iterator/iteratorIntersect.hpp"
#include <stdexcept>

using namespace strus;

IteratorIntersect::IteratorIntersect( const IteratorReference& first_, const IteratorReference& second_)
	:m_docno(0)
	,m_first(first_)
	,m_second(second_)
{}

IteratorIntersect::IteratorIntersect( const IteratorIntersect& o)
	:m_docno(o.m_docno)
	,m_first(o.m_first->copy())
	,m_second(o.m_second->copy())
{
	m_featureid.append( m_first->featureid());
	m_featureid.append( m_second->featureid());
	m_featureid.push_back( 'I');
}

std::vector<const IteratorInterface*> IteratorIntersect::subExpressions( bool positive)
{
	std::vector<const IteratorInterface*> rt;
	if (positive)
	{
		rt.push_back( m_first.get());
		rt.push_back( m_second.get());
	}
	return rt;
}

Index IteratorIntersect::skipDoc( const Index& docno_)
{
	Index docno_iter = docno_;

	for (;;)
	{
		Index docno_first = m_first->skipDoc( docno_iter);
		Index docno_second = m_second->skipDoc( docno_first);

		if (!docno_first || !docno_second)
		{
			return 0;
		}
		if (docno_first == docno_second)
		{
			m_docno = docno_first;
			return m_docno;
		}
		docno_iter = docno_second;
	}
}

Index IteratorIntersect::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;
	if (!m_docno) return 0;

	for (;;)
	{
		Index pos_first = m_first->skipPos( pos_iter);
		Index pos_second = m_second->skipPos( pos_first);

		if (!pos_first || !pos_second)
		{
			return 0;
		}
		if (pos_first == pos_second)
		{
			return pos_first;
		}
		pos_iter = pos_second;
	}
}

