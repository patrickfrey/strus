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

IteratorIntersect::IteratorIntersect( std::size_t nofargs, const IteratorInterface** args)
	:m_docno(0)
{
	std::size_t ii=0;
	m_argar.reserve( nofargs);
	for (; ii<nofargs; ++ii)
	{
		if (args[ii])
		{
			m_argar.push_back( args[ii]->copy());
			m_featureid.append( args[ii]->featureid());
		}
	}
	m_featureid.push_back( 'I');
}

IteratorIntersect::IteratorIntersect( const IteratorIntersect& o)
	:m_docno(o.m_docno)
	,m_featureid(o.m_featureid)
{
	m_argar.reserve( o.m_argar.size());
	std::size_t ii=0;
	for (; ii<o.m_argar.size(); ++ii)
	{
		if (o.m_argar[ii].get())
		{
			m_argar.push_back( o.m_argar[ ii]->copy());
		}
	}
}

std::vector<const IteratorInterface*> IteratorIntersect::subExpressions( bool positive)
{
	std::vector<const IteratorInterface*> rt;
	if (positive)
	{
		rt.reserve( m_argar.size());
		std::size_t ii=0;
		for (; ii<m_argar.size(); ++ii)
		{
			rt.push_back( m_argar[ ii].get());
		}
	}
	return rt;
}

Index IteratorIntersect::skipDoc( const Index& docno_)
{
	Index docno_iter = docno_;
	for (;;)
	{
		std::vector<IteratorReference>::const_iterator ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		docno_iter = (*ai)->skipDoc( docno_iter);
		if (docno_iter == 0)
		{
			m_docno = 0;
			return 0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index docno_next = (*ai)->skipDoc( docno_iter);
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
	if (!m_docno) return 0;

	Index pos_iter = pos_;
	for (;;)
	{
		std::vector<IteratorReference>::const_iterator ai = m_argar.begin(), ae = m_argar.end();
		if (ai == ae) return 0;

		pos_iter = (*ai)->skipPos( pos_iter);
		if (pos_iter == 0)
		{
			return 0;
		}
		for (++ai; ai != ae; ++ai)
		{
			Index pos_next = (*ai)->skipPos( pos_iter);
			if (pos_next != pos_iter)
			{
				if (pos_next == 0) return 0;
				pos_iter = pos_next;
				break;
			}
		}
		if (ai == ae)
		{
			return pos_iter;
		}
	}
}

