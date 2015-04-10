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
#ifndef _STRUS_SUMMARIZATION_FEATURE_HPP_INCLUDED
#define _STRUS_SUMMARIZATION_FEATURE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include <vector>
#include <string>

namespace strus
{

/// \brief Structure for a feature used for summarization
class SummarizationFeature
{
public:
	/// \brief Constructor
	/// \param[in] postingIterator_ iterator on the occurrencies in the document
	/// \param[in] variables_ variables attached to this summarization feature
	SummarizationFeature(
			PostingIteratorInterface* postingIterator_,
			const std::vector<SummarizationVariable>& variables_)
		:m_postingIterator(postingIterator_)
		,m_variables(variables_)
	{}
	/// \brief Copy constructor
	/// \param[in] o summarization feature to copy
	SummarizationFeature( const SummarizationFeature& o)
		:m_postingIterator(o.m_postingIterator)
		,m_variables(o.m_variables)
	{}

	/// \brief Destructor
	~SummarizationFeature(){}

	/// \brief Get the iterator on the postings (feature occurrencies in the document)
	/// \return the posting iterator
	PostingIteratorInterface* postingIterator() const		{return m_postingIterator;}

	/// \brief Iterator on the variables attached
	typedef std::vector<SummarizationVariable>::const_iterator variable_const_iterator;

	/// \brief Get the start iterator on the variables attached to this feature
	variable_const_iterator variables_begin() const			{return m_variables.begin();}
	variable_const_iterator variables_end()	const			{return m_variables.end();}

	/// \brief Get the list of all variables
	const std::vector<SummarizationVariable>& variables() const	{return m_variables;}

private:
	PostingIteratorInterface* m_postingIterator;			///< iterator on postings
	std::vector<SummarizationVariable> m_variables;			///< variables attached
};

}//namespace
#endif


