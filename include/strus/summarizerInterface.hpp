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
#ifndef _STRUS_SUMMARIZER_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class IteratorInterface;
/// \brief Forward declaration
class ForwardIndexViewerInterface;


/// \brief Interface for implementing summarization (additional info about the matches in the result ranklist of a retrieval query)
class SummarizerInterface
{
public:
	virtual ~SummarizerInterface(){}

	/// \brief Element of summarization
	class SummaryElement
	{
	public:
		SummaryElement(
				const std::string& text_,
				Index pos_=0,
				unsigned int length_=0)
			:m_text(text_)
			,m_pos(pos_)
			,m_length(length_){}

		SummaryElement( const SummaryElement& o)
			:m_text(o.m_text)
			,m_pos(o.m_pos)
			,m_length(o.m_length){}

		Index pos() const			{return m_pos;}
		unsigned int length() const		{return m_length;}
		const std::string& text() const		{return m_text;}

	private:
		std::string m_text;
		Index m_pos;
		unsigned int m_length;
	};

	/// \brief Get the summarization based on term occurrencies
	/// \param[in,out] res where to append the summarization result
	/// \param[in] docno document to get the summary element from or 0, if the summary should be global
	/// \param[in] pos position tp get the summary element from element or 0, if the summary should be for the whole document
	/// \param[in] itr iterator for the term occurrencies where to get the summary from
	/// \param[in] markitr iterator for context markers related to the summary
	/// \return the summarization elements
	virtual bool
		getSummary(
			std::vector<SummaryElement>& res,
			const Index& docno,
			const Index& pos,
			IteratorInterface& itr,
			IteratorInterface& markitr)=0;
};

}//namespace
#endif


