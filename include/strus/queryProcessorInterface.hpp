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
#ifndef _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class SummarizerInterface;


/// \brief Defines all object instances involved in query evaluation addressable by name
class QueryProcessorInterface
{
public:
	/// \brief Destructor
	virtual ~QueryProcessorInterface(){}

	/// \brief Create an iterator on the occurrencies of a term
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference object (to dispose with 'delete')
	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value) const=0;

	/// \brief Create an iterator as join function on the arguments passed
	/// \param[in] name name of the join function to execute
	/// \param[in] range additional parameters for describing a range or offset for the join operator to execute
	/// \param[in] nofargs number of arguments to pass to the function
	/// \param[in] args arguments to pass to the function
	/// \return the created iterator reference object representing the result of the function (to dispose with 'delete')
	virtual PostingIteratorInterface*
		createJoinPostingIterator(
			const std::string& name,
			int range,
			std::size_t nofargs,
			const PostingIteratorInterface** args) const=0;

	/// \brief Create a weighting function for term occurrencies
	/// \param[in] name name of the weighting function
	/// \param[in] parameter scaling arguments for the function
	/// \return the created weighting function object (to dispose with 'delete')
	virtual WeightingFunctionInterface*
		createWeightingFunction(
			const std::string& name,
			const std::vector<float>& parameter) const=0;

	/// \brief Create a summarizer for adding attributes to matches
	/// \param[in] name name of the summarizer
	/// \param[in] type term type or attribute name (depends on implementation)
	/// \param[in] parameter scalar arguments for the summarizer
	/// \param[in] structitr iterator representing the structure delimiter elements for summarization
	/// \param[in] nofitrs number of feature iterators this summarizer is based on
	/// \param[in] itrs feature iterators for this summarizer 
	/// \return the created summarizer object (to dispose with 'delete')
	virtual SummarizerInterface*
		createSummarizer(
			const std::string& name,
			const std::string& type,
			const std::vector<float>& parameter,
			const PostingIteratorInterface* structitr,
			std::size_t nofitrs,
			const PostingIteratorInterface** itrs) const=0;
};

}//namespace
#endif

