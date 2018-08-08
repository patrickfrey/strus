/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for building up metadata restrictions
#ifndef _STRUS_METADATA_RESTRICTION_INTERFACE_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_INTERFACE_HPP_INCLUDED
#include "strus/numericVariant.hpp"
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

	/// \brief Get the operator as string
	static const char* compareOperatorStr( CompareOperator op)
	{
		static const char* ar[] = {"<","<=","==","!=",">",">=",0};
		return ar[ (int)op];
	}
	/// \brief Get the operator name as string
	static const char* compareOperatorName( CompareOperator op)
	{
		static const char* ar[] = {"lt","le","eq","ne","gt","ge",0};
		return ar[ (int)op];
	}

	/// \brief Add a condition on the metadata to this metadata restriction
	/// \param[in] opr condition compare operator
	/// \param[in] name name of meta data element to check
	/// \param[in] operand constant number to check against
	/// \param[in] newGroup true, if the conditional opens a new group of elements joined with a logical "OR" 
	///			false, if the conditional belongs to the last group of elements joined with a logical "OR".
	///		Different groups are joined with a logical "AND" to form the meta data restriction expression
	///		(See CNF = conjunctive normalform)
	virtual void addCondition(
			const CompareOperator& opr,
			const std::string& name,
			const NumericVariant& operand,
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

