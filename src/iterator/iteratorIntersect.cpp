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
#include "strus/iteratorIntersect.hpp"

using namespace strus;

IteratorIntersect::IteratorIntersect( const IteratorInterfaceR& first_, const IteratorInterfaceR& second_)
	:m_docno(0)
	,m_first(first_)
	,m_second(second_)
{}

Index IteratorIntersect::skipDoc( const Index& docno_)
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
				return m_docno;
			}
		}
		else
		{
			docno_iter = docno_second;
		}
	}
}

Index IteratorIntersect::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;

	for (;;)
	{
		Index pos_first = m_first->skipDoc( pos_iter);
		Index pos_second = m_second->skipDoc( pos_iter);

		if (!pos_first || !pos_second)
		{
			return 0;
		}
		if (pos_first >= pos_second)
		{
			if (pos_first == pos_second)
			{
				return pos_first;
			}
			pos_iter = pos_first;
		}
		else
		{
			pos_iter = pos_second;
		}
	}
}


