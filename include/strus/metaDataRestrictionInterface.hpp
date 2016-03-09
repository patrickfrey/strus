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
/// \brief Interface for building up metadata restrictions
#ifndef _STRUS_METADATA_RESTRICTION_INTERFACE_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_INTERFACE_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/index.hpp"
#include <string>

namespace strus {

/// \brief Forward declaration
class MetaDataRestrictionInstanceInterface;

/// \brief Class for building up a metadata restriction
class MetaDataRestrictionInterface
{
public:
	/// \brief Destructor
	virtual ~MetaDataRestrictionInterface()
	{}

	/// \brief Comparison operator for restrictions
	enum CompareOperator
	{
		CompareLess,		///< operator '<' respecting machine epsilon
		CompareLessEqual,	///< operator '<=' respecting machine epsilon
		CompareEqual,		///< operator '==' respecting machine epsilon
		CompareNotEqual,	///< operator '!=' respecting machine epsilon
		CompareGreater,		///< operator '>' respecting machine epsilon
		CompareGreaterEqual	///< operator '>=' respecting machine epsilon
	};
	/// \brief Number of comparison operators
	enum {NofCompareOperators=((int)CompareGreaterEqual+1)};

	/// \brief Add a condition on the metadata to this metadata restriction
	/// \param[in] opr condition compare operator
	/// \param[in] name name of meta data element to check
	/// \param[in] operand constant number to check against
	/// \param[in] newGroup true, if the conditional opens a new group of elements joined with a logical "OR" 
	///			false, if the conditional belongs to the last group of elements joined with a logical "OR".
	///		Different groups are joined with a logical "AND" to form the meta data restriction expression
	///		(See CNF = conjunctive normalform)
	virtual void addCondition(
			CompareOperator opr,
			const std::string& name,
			const ArithmeticVariant& operand,
			bool newGroup=true)=0;

	/// \brief Create an instance of this metadata restriction
	/// \return the created restriction instance
	virtual MetaDataRestrictionInstanceInterface* createInstance() const=0;

	/// \brief Return a readable string representation of the expression
	/// \return the expression as string
	virtual std::string tostring() const=0;
};

} //namespace
#endif

