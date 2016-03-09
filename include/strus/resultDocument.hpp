/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Result element of the query evaluation
/// \file "resultDocument.hpp"
#ifndef _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#define _STRUS_RESULT_DOCUMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/weightedDocument.hpp"
#include "strus/summaryElement.hpp"
#include <vector>

namespace strus {

/// \class ResultDocument
/// \brief Structure defining one result element of a strus query
class ResultDocument
	:public WeightedDocument
{
public:
	/// \brief Default constructor
	ResultDocument(){}
	/// \brief Copy constructor
	ResultDocument( const ResultDocument& o)
		:WeightedDocument(o),m_summaryElements(o.m_summaryElements){}
	/// \brief Constructor from a composition of the pure query evaluation result and the summary elements
	ResultDocument( const WeightedDocument& o, const std::vector<SummaryElement>& summaryElements_)
		:WeightedDocument(o),m_summaryElements(summaryElements_){}
	/// \brief Constructor from a composition of the pure query evaluation result without summary elements
	ResultDocument( const WeightedDocument& o)
		:WeightedDocument(o){}
	/// \brief Constructor from a composition its basic parts
	ResultDocument( const Index& docno_, float weight_, const std::vector<SummaryElement>& summaryElements_)
		:WeightedDocument(docno_,weight_),m_summaryElements(summaryElements_){}

	/// \brief Get the list of summary elements of this result
	const std::vector<SummaryElement>& summaryElements() const	{return m_summaryElements;}

private:
	std::vector<SummaryElement> m_summaryElements;	///< summary elements of this result
};

}//namespace
#endif

