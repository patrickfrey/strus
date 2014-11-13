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
#include "databaseKey.hpp"
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <leveldb/db.h>

using namespace strus;

DocnoBlock::DocnoBlock()
	:m_ar(0),m_arsize(0){}

DocnoBlock::DocnoBlock( const Element* ar_, std::size_t arsize_)
	:m_ar(ar_)
	,m_arsize(arsize_)
{}

DocnoBlock::DocnoBlock( const DocnoBlock& o)
	:m_ar(o.m_ar)
	,m_arsize(o.m_arsize)
{}

void DocnoBlock::init( const Element* ar_, std::size_t arsize_)
{
	m_ar = ar_;
	m_arsize = arsize_;
}

const DocnoBlock::Element* DocnoBlock::upper_bound( const Index& docno_) const
{
	std::size_t first=0,last=m_arsize;
	while (first+4 < last)
	{
		std::size_t mid = (first + last) >> 1;
		Index dn = m_ar[ mid].docno();
		if (dn < docno_)
		{
			first = mid+1;
		}
		else if (dn > docno_)
		{
			last = mid+1;
		}
		else
		{
			return m_ar + mid;
		}
	}
	for (;first < last; ++first)
	{
		if (m_ar[ first].docno() >= docno_)
		{
			return m_ar + first;
		}
	}
	return 0;
}

const DocnoBlock::Element* DocnoBlock::find( const Index& docno_) const
{
	const Element* rt = upper_bound( docno_);
	if (rt && rt->docno() == docno_) return rt;
	return 0;
}


DocnoBlock::Element::Element( Index docno_, unsigned int ff_, float weight_)
	:m_docno((uint32_t)docno_)
	,m_ff(ff_>Max_ff?Max_ff:ff_)
	,m_weight(floatSingleToHalfPrecision(weight_))
{
	if (docno_ > std::numeric_limits<uint32_t>::max()) 
	{
		throw std::runtime_error( "document number out of range");
	}
}

DocnoBlock::Element::Element( const Element& o)
	:m_docno(o.m_docno)
	,m_ff(o.m_ff)
	,m_weight(o.m_weight){}

float DocnoBlock::Element::weight() const
{
	return floatHalfToSinglePrecision( m_weight);
}



