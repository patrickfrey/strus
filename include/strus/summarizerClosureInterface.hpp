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
#ifndef _STRUS_SUMMARIZER_CLOSURE_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_CLOSURE_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for the summarization context (of a SummarizationFunction)
class SummarizerClosureInterface
{
public:
	/// \brief One result element of summarization
	class SummaryElement
	{
	public:
		/// \brief Constructor
		SummaryElement( const std::string& text_, float weight_=1.0)
			:m_text(text_),m_weight(weight_){}
		/// \brief Copy constructor
		SummaryElement( const SummaryElement& o)
			:m_text(o.m_text),m_weight(o.m_weight){}

		/// \brief Content value of the element
		const std::string& text() const		{return m_text;}
		/// \brief Weight of the element if defined
		float weight() const			{return m_weight;}

	private:
		std::string m_text;	///< content value of the element
		float m_weight;		///< weight of the element if defined
	};

public:
	/// \brief Destructor
	virtual ~SummarizerClosureInterface(){}

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<SummaryElement> getSummary( const Index& docno)=0;
};

}//namespace
#endif


