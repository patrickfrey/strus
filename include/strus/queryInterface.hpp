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
/// \brief Interface for defining a query and evaluating it.
/// \file "queryInterface.hpp"
#ifndef _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include "strus/resultDocument.hpp"
#include "strus/arithmeticVariant.hpp"

namespace strus {

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class PostingJoinOperatorInterface;

/// \brief Defines a strus information retrieval query
class QueryInterface
{
public:
	/// \brief Destructor
	virtual ~QueryInterface(){}

	/// \brief Push a term to the query stack
	/// \param[in] type_ term type
	/// \param[in] value_ term value
	virtual void pushTerm( const std::string& type_, const std::string& value_)=0;

	/// \brief Push an expression formed by the topmost elements from the stack to the query stack,
	///	removing the argument elements.
	/// \param[in] operation the expression join operator
	/// \param[in] argc number of expression arguments
	/// \param[in] range range of the expression
	/// \param[in] cardinality required size of matching results (e.g. minimum number of elements of any input subset selection that builds a result) (0 for use default)
	virtual void pushExpression(
				const PostingJoinOperatorInterface* operation,
				std::size_t argc, int range, unsigned int cardinality)=0;

	/// \brief Push a duplicate of the topmost element(s) of the query stack
	/// \param[in] argc number of arguments to duplicate
	/// \note This function makes it possible to reference terms or expressions more than once as features or as subexpressions.
	virtual void pushDuplicate( std::size_t argc=1)=0;

	/// \brief Attaches a variable to the top expression or term on the query stack.
	/// \note The positions of the query matches of the referenced term or expression can be accessed through this variable in summarization.
	/// \param[in] name_ name of the variable attached
	/// \remark The stack is not changed
	/// \remark More than one variable can be attached
	virtual void attachVariable( const std::string& name_)=0;

	/// \brief Define the topmost element of the stack as feature, removing it from the stack
	/// \param[in] set_ name of the set of the new feature created
	/// \param[in] weight_ weight of the feature for the weighting function in query evaluation 
	virtual void defineFeature( const std::string& set_, float weight_=1.0)=0;

	/// \brief Comparison operator for restrictions
	enum CompareOperator
	{
		CompareLess,
		CompareLessEqual,
		CompareEqual,
		CompareNotEqual,
		CompareGreater,
		CompareGreaterEqual
	};
	enum {NofCompareOperators=((int)CompareGreaterEqual+1)};

	/// \brief Define a restriction on documents base on a condition on the meta data
	/// \param[in] opr condition compare operator
	/// \param[in] name name of meta data element to check
	/// \param[in] operand constant number to check against
	/// \param[in] newGroup true, if the conditional opens a new group of elements joined with a logical "OR" 
	///			false, if the conditional belongs to the last group of elements joined with a logical "OR".
	///		Different groups are joined with a logical "AND" to form the meta data restriction expression
	virtual void defineMetaDataRestriction(
			CompareOperator opr, const std::string& name,
			const ArithmeticVariant& operand, bool newGroup=true)=0;

	/// \brief Define a restriction on the documents as list of local document numbers (Add local document numbers to the list of documents to restrict the query on)
	/// \param[in] docnolist_ list of documents to evaluate the query on
	virtual void addDocumentEvaluationSet(
			const std::vector<Index>& docnolist_)=0;

	/// \brief Set the maximum number of ranks to evaluate starting with the minimum rank
	/// \param[in] maxNofRanks_ maximum number of ranks
	virtual void setMaxNofRanks( std::size_t maxNofRanks_)=0;
	/// \brief Set the minimum rank number to return
	/// \param[in] minRank_ the minimum rank number
	virtual void setMinRank( std::size_t minRank_)=0;
	/// \brief Add a name of a user role in the query for alternative ACL restrictions
	/// \param[in] username_ user of the query
	virtual void addUserName( const std::string& username_)=0;

	/// \brief Evaluate the query
	virtual std::vector<ResultDocument> evaluate()=0;
};

}//namespace
#endif
