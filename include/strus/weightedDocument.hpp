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
#ifndef _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#define _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include <utility>

namespace strus {

class WeightedDocument
{
public:
	WeightedDocument()
		:m_docno(0),m_weight(0.0){}
	WeightedDocument( const WeightedDocument& o)
		:m_docno(o.m_docno),m_weight(o.m_weight){}
	WeightedDocument( const Index& docno_, float weight_)
		:m_docno(docno_),m_weight(weight_){}

	Index docno() const					{return m_docno;}
	float weight() const					{return m_weight;}

	bool operator < ( const WeightedDocument& o) const
	{
		return (m_weight < o.m_weight);
	}
	bool operator > ( const WeightedDocument& o) const
	{
		return (m_weight > o.m_weight);
	}

private:
	Index m_docno;
	float m_weight;
};

}//namespace
#endif

