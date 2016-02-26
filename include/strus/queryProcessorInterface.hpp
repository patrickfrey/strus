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
/// \brief Interface for the container of all types of functions provided for query evaluation.
/// \file queryProcessorInterface.hpp
#ifndef _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class AttributeReaderInterface;

/// \brief Defines all object instances involved in query evaluation addressable by name
class QueryProcessorInterface
{
public:
	/// \brief Destructor
	virtual ~QueryProcessorInterface(){}

	/// \brief Define a new posting set join operation
	/// \param[in] name the name of the function
	/// \param[in] op the function reference (ownership passed to this)
	virtual void
		definePostingJoinOperator(
			const std::string& name,
			PostingJoinOperatorInterface* op)=0;

	/// \brief Get a join function reference defined by 'name'
	/// \param[in] name name of the join function to get
	/// \return the join function object reference
	virtual const PostingJoinOperatorInterface*
		getPostingJoinOperator(
			const std::string& name) const=0;

	/// \brief Define a new weighting function
	/// \param[in] name the name of the function
	/// \param[in] func the function to define (ownership passed to this)

	virtual void
		defineWeightingFunction(
			const std::string& name,
			WeightingFunctionInterface* func)=0;

	/// \brief Get a weighting function reference by name
	/// \param[in] name name of the weighting function
	/// \return the weighting function object reference
	virtual const WeightingFunctionInterface*
		getWeightingFunction(
			const std::string& name) const=0;

	/// \brief Define a new summarization function
	/// \param[in] name name of the summarization function
	/// \param[in] sumfunc the summarization function object reference (ownership passed to this)
	virtual void
		defineSummarizerFunction(
			const std::string& name,
			SummarizerFunctionInterface* sumfunc)=0;

	/// \brief Get a summarization function reference by name
	/// \param[in] name name of the summarization function
	/// \return the summarization function object reference
	virtual const SummarizerFunctionInterface*
		getSummarizerFunction(
			const std::string& name) const=0;

	/// \brief Function type for fetching the list of available functions
	enum FunctionType
	{
		PostingJoinOperator,		///< Addresses a posting iterator join operator
		WeightingFunction,		///< Addresses a weighting function
		SummarizerFunction		///< Addresses a summarization function
	};

	/// \brief Get a list of all functions of a specific type available
	/// \param[in] type type of the function
	/// \return the list of function names
	virtual std::vector<std::string> getFunctionList( FunctionType type) const=0;
};

}//namespace
#endif

