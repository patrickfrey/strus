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
#include "iterator/postingIteratorDifference.hpp"
#include <stdexcept>

using namespace strus;

IteratorDifference::IteratorDifference(
		const Reference<PostingIteratorInterface>& positive_,
		const Reference<PostingIteratorInterface>& negative_)
	:m_docno(0)
	,m_docno_neg(0)
	,m_positive( positive_)
	,m_negative( negative_)
{
	m_featureid.append( positive_->featureid());
	m_featureid.append( negative_->featureid());
	m_featureid.push_back( 'N');
}

IteratorDifference::~IteratorDifference()
{}

std::vector<const PostingIteratorInterface*> IteratorDifference::subExpressions( bool positive) const
{
	std::vector<const PostingIteratorInterface*> rt;
	if (positive)
	{
		rt.push_back( m_positive.get());
	}
	else
	{
		rt.push_back( m_negative.get());
	}
	return rt;
}

Index IteratorDifference::skipDoc( const Index& docno_)
{
	m_docno = m_positive->skipDoc( docno_);
	m_docno_neg = m_negative->skipDoc( m_docno);
	return m_docno;
}

Index IteratorDifference::skipPos( const Index& pos_)
{
	Index pos_iter = pos_;
	if (!m_docno) return 0;

	for (;;)
	{
		Index pos_positive = m_positive->skipPos( pos_iter);
		if (!pos_positive) return 0;
		if (m_docno_neg == m_docno)
		{
			Index pos_negative = m_negative->skipPos( pos_positive);
			if (pos_negative == pos_positive)
			{
				pos_iter = pos_positive+1;
				continue;
			}
		}
		return pos_positive;
	}
}

