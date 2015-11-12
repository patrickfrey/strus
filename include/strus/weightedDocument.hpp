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
/// \brief Weighted document of the query evaluation (result document without attributes)
/// \file weightedDocument.hpp
#ifndef _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#define _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include <utility>
#include <cmath>

namespace strus {

/// \class WeightedDocument
/// \brief Pure ranking result of a strus query without the attributes
class WeightedDocument
{
public:
	/// \brief Default constructor
	WeightedDocument()
		:m_docno(0),m_weight(0.0){}
	/// \brief Copy constructor
	WeightedDocument( const WeightedDocument& o)
		:m_docno(o.m_docno),m_weight(o.m_weight){}
	/// \brief Constructor
	WeightedDocument( const Index& docno_, float weight_)
		:m_docno(docno_),m_weight(weight_){}

	/// \brief Get the document number of the result
	Index docno() const					{return m_docno;}
	/// \brief Get the accumulated weight of the ranking of the result
	float weight() const					{return m_weight;}

	/// \brief Comparison for sorting
	bool operator < ( const WeightedDocument& o) const
	{
		double diff = m_weight - o.m_weight;
		if (fabs( diff) < std::numeric_limits<double>::epsilon())
		{
			return m_docno < o.m_docno;
		}
		else
		{
			return (diff < 0.0);
		}
	}
	/// \brief Comparison for sorting
	bool operator > ( const WeightedDocument& o) const
	{
		return (m_weight > o.m_weight);
	}

private:
	Index m_docno;			///< document number
	float m_weight;			///< accumulated ranking weight
};

}//namespace
#endif

