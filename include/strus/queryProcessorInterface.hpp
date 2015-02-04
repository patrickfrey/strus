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
#include <string>

namespace strus
{
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class PostingJoinOperatorInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class AttributeReaderInterface;

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

	/// \brief Define a new posting set join operation
	/// \param[in] name the name of the function
	/// \param[in] func the function reference
	virtual void
		definePostingJoinOperator(
			const char* name,
			const PostingJoinOperatorInterface* op)=0;

	/// \brief Get a join function reference defined by 'name'
	/// \param[in] name name of the join function to get
	/// \return the join function object reference
	virtual const PostingJoinOperatorInterface*
		getPostingJoinOperator(
			const std::string& name) const=0;

	/// \brief Define a new weighting function
	/// \param[in] name the name of the function
	/// \param[in] func the function to define
	virtual void
		defineWeightingFunction(
			const char* name,
			const WeightingFunctionInterface* func)=0;

	/// \brief Get a weighting function reference by name
	/// \param[in] name name of the weighting function
	/// \return the weighting function object reference
	virtual const WeightingFunctionInterface*
		getWeightingFunction(
			const std::string& name) const=0;

	/// \brief Define a new summarization function
	/// \param[in] name name of the summarization function
	/// \param[in] the summarization function object reference
	virtual void
		defineSummarizerFunction(
			const char* name,
			const SummarizerFunctionInterface* sumfunc)=0;

	/// \brief Get a summarization function reference by name
	/// \param[in] name name of the summarization function
	/// \return the summarization function object reference
	virtual const SummarizerFunctionInterface*
		getSummarizerFunction(
			const std::string& name) const=0;
};

}//namespace
#endif

