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
class StorageInterface;

/// \brief Defines a strus information retrieval query
class QueryInterface
{
public:
	/// \brief Destructor
	virtual ~QueryInterface(){}

	/// \brief Print the contents of this query in readable form
	/// \param[out] where to print to
	virtual void print( std::ostream& out)=0;

	/// \brief Push a term to the query stack
	/// \param[in] type_ term type
	/// \param[in] value_ term value
	virtual void pushTerm( const std::string& type_, const std::string& value_)=0;

	/// \brief Push an expression formed by the topmost elements from the stack to the query stack,
	///	removing the argument elements.
	/// \param[in] opname_ name of the expression join operator
	/// \param[in] argc number of expression arguments
	/// \param[in] range_ range of the expression
	virtual void pushExpression( const std::string& opname_, std::size_t argc, int range_)=0;

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
			CompareOperator opr, const char* name,
			const ArithmeticVariant& operand, bool newGroup=true)=0;

	/// \brief Define a restriction on documents that contain a feature of a certain feature set
	/// \param[in] set_ name of the set of the restriction feature
	virtual void defineFeatureRestriction( const std::string& set_)=0;

	/// \brief Set the maximum number of ranks to evaluate
	/// \param[in] maxNofRanks_ maximum number of ranks
	virtual void setMaxNofRanks( std::size_t maxNofRanks_)=0;
	/// \brief Set the minimum rank number to return
	/// \param[in] minRank_ the minimum rank number
	virtual void setMinRank( std::size_t minRank_)=0;
	/// \brief Set the name of the user of the query for ACL restrictions
	/// \param[in] username_ user of the query
	virtual void setUserName( const std::string& username_)=0;

	/// \brief Evaluate the query
	virtual std::vector<ResultDocument> evaluate()=0;
};

}//namespace
#endif
