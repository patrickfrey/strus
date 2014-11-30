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
#include "docnoBlock.hpp"
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <leveldb/db.h>

using namespace strus;

DocnoBlockElement::DocnoBlockElement( Index docno_, unsigned int ff_, float weight_)
	:m_docno(docno_)
	,m_ff(ff_>Max_ff?Max_ff:ff_)
	,m_weight(floatSingleToHalfPrecision(weight_))
{}

DocnoBlockElement::DocnoBlockElement( const DocnoBlockElement& o)
	:m_docno(o.m_docno)
	,m_ff(o.m_ff)
	,m_weight(o.m_weight){}

float DocnoBlockElement::weight() const
{
	return floatHalfToSinglePrecision( m_weight);
}

std::ostream& strus::operator<< (std::ostream& out, const DocnoBlockElement& e)
{
	out << "docno=" << e.docno() << ",ff=" << e.ff() << ",weight=" << e.weight();
	return out;
}


const DocnoBlockElement* DocnoBlock::upper_bound( const Index& docno_, const DocnoBlockElement* lowerbound) const
{
	std::size_t first = lowerbound-begin(), last = nofElements();
	std::size_t mid = first + ((last - first) >> 4);

	while (first+4 < last)
	{
		Index dn = ptr()[ mid].docno();
		if (dn < docno_)
		{
			first = mid+1;
			mid = (first + last) >> 1;
		}
		else if (dn > docno_)
		{
			last = mid+1;
			mid = (first + last) >> 1;
		}
		else
		{
			return ptr() + mid;
		}
	}
	for (;first < last; ++first)
	{
		if (ptr()[ first].docno() >= docno_)
		{
			return ptr() + first;
		}
	}
	return 0;
}

const DocnoBlockElement* DocnoBlock::find( const Index& docno_, const DocnoBlockElement* lowerbound) const
{
	const DocnoBlockElement* rt = upper_bound( docno_, lowerbound);
	if (rt && rt->docno() == docno_) return rt;
	return 0;
}

DocnoBlock DocnoBlock::merge( const DocnoBlock& newblk, const DocnoBlock& oldblk)
{
	DocnoBlock rt;
	Index blkid = (oldblk.id() > newblk.id())?oldblk.id():newblk.id();
	rt.setId( blkid);

	const DocnoBlockElement* ai = newblk.begin();
	const DocnoBlockElement* ae = newblk.end();
	const DocnoBlockElement* bi = oldblk.begin();
	const DocnoBlockElement* be = oldblk.end();
	while (ai != ae && bi != be)
	{
		if (ai->docno() <= bi->docno())
		{
			if (!ai->empty())
			{
				rt.push_back( *ai);
			}
			if (ai->docno() == bi->docno())
			{
				//... defined twice -> prefer new entry and ignore old
				++bi;
			}
			++ai;
		}
		else
		{
			rt.push_back( *bi);
			++bi;
		}
	}
	while (ai != ae)
	{
		if (!ai->empty())
		{
			rt.push_back( *ai);
		}
		++ai;
	}
	while (bi != be)
	{
		if (!bi->empty())
		{
			rt.push_back( *bi);
		}
		++bi;
	}
	return rt;
}




